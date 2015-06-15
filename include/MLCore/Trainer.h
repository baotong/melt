/**
 *  ==============================================================================
 *
 *          \file   MLCore/Trainer.h
 *
 *        \author   chenghuige
 *
 *          \date   2014-04-06 20:47:13.472915
 *
 *  \Description:
 *  ==============================================================================
 */

#ifndef M_L_CORE__TRAINER_H_
#define M_L_CORE__TRAINER_H_

#include "common_def.h"
#include "random_util.h"
#include "Prediction/Instances/Instances.h"
#include "MLCore/Predictor.h"
#include "Prediction/Normalization/Normalizer.h"
#include "Prediction/Calibrate/Calibrator.h"
#include "Utils/Evaluator.h"
#include "statistic_util.h"
#include "MLCore/LossKind.h"

namespace gezi {

	class Trainer : public WithArgs, public WithHelp
	{
	public:
		virtual PredictionKind GetPredictionKind()
		{
			return PredictionKind::BinaryClassification;
		}

		virtual LossKind GetLossKind()
		{
			return LossKind::Unknown;
		}

		//Ĭ�������ر�Ϊ�������Է�����Ҫnormalize������׼�� �������Ҫ����Fastrank �����Լ�override
		virtual void Train(Instances& instances)
		{
			VLOG(5) << "Train in trainer base";
			_trainingSchema = instances.schema;
			_instances = &instances;

			_numFeatures = instances.NumFeatures();
			_featureNames = instances.schema.featureNames;

			Init();
			Initialize(instances);

			TryInitializeNormalizer(instances, _isStreaming); //normalize and set _instances

			InnerTrain(*_instances);

			Finalize(*_instances);

			if (!GetParam().empty())
			{
				VLOG(0) << "Param: [" << GetParam() << " ]" << endl;
			}
		}

		virtual void TryInitializeNormalizer(Instances& instances, bool isStreaming)
		{
			//--- ���������ݹ�һ�� ��TLC���Բ�ͬ TLC��normalize����ѵ��������(��Ҫ�����Ǽ���streamingģʽ)
			//�ر���hadoop scopeѵ��  @TODO  Ҳ������Ҳ��仯
			//����������ƽ�����֤ �������ѵ������ Ĭ���� no normalize copy
			//Ŀǰ���־���ȫ��normalize ������Щ��� ����ѵ�������ȡһС���� ��ôѵ��������normalize���ܸ�����@TODO
			if (!isStreaming && _normalizer != nullptr && !instances.IsNormalized())
			{
				if (!_normalizeCopy)
				{
					_normalizer->RunNormalize(instances);
				}
				else
				{
					normalizedInstances() = _normalizer->RunNormalizeCopy(instances);
					_instances = &normalizedInstances();
				}
			}
		}

		const HeaderSchema& TrainingSchema() const
		{
			return _trainingSchema;
		}

		virtual PredictorPtr CreatePredictor() = 0; //@TODO Ҳ��Ӧ������ǿ�Ƽ���Predictor::SetParam

		virtual string GetParam()
		{
			return "";
		}

		RandomPtr GetRandom()
		{
			return _rand;
		}

		NormalizerPtr GetNormalizer()
		{
			return _normalizer;
		}

		CalibratorPtr GetCalibrator()
		{
			return _calibrator;
		}

		void SetNormalizeCopy(bool normalizeCopy = true)
		{
			_normalizeCopy = normalizeCopy;
		}


		void SetNormalizer(NormalizerPtr normalizer)
		{
			_normalizer = normalizer;
		}

		void SetCalibrator(CalibratorPtr calibrator)
		{
			_calibrator = calibrator;
		}

		void SetStreaming()
		{
			_isStreaming = true;
		}

		virtual unsigned GetRandSeed() const
		{
			return _randSeed;
		}

		void SetRandSeed(unsigned randSeed)
		{
			_randSeed = randSeed;
		}

	protected:
		virtual void Init()
		{

		}

		virtual void Initialize(Instances& instances)
		{

		}
		virtual void InnerTrain(Instances& instances)
		{

		}

		virtual void Finalize(Instances& instances)
		{

		}
	protected:
		HeaderSchema _trainingSchema;

		bool _isStreaming = false;
		unsigned _randSeed = 0;
		RandomPtr _rand = nullptr;
		RandomRangePtr _randRange = nullptr;
		NormalizerPtr _normalizer = nullptr;
		CalibratorPtr _calibrator = nullptr;

		FeatureNamesVector _featureNames;
		/// <summary> Total number of features </summary>
		int _numFeatures = 0;

		bool _normalizeCopy = false;

		//ϣ����ͬ������Trainer Share��ͬ��normalizedInstance ���ǲ�ͬ���̸߳���һ��,���ⲻ�õ�norm�򲻲���_normalizedInstance
		//@TODO ����Ҳ���Ǻܴ󡣡�  ֱ����ͨ Instances _normalizedInstances; Ӧ��Ҳok�� ���������normalize Ӧ���ȶ���������norm��
		//��������һ�������ǲ�ϣ��һ���̵߳Ķ����ͬTrainerʵ���ж���ڴ�ռ�ô洢_normalizedInstances
		static Instances& normalizedInstances()
		{
			static thread_local Instances _normalizedInstances;
			return _normalizedInstances;
		}

		Instances* _instances = NULL;
	};

	typedef shared_ptr<Trainer> TrainerPtr;

	class ValidatingTrainer : public Trainer
	{
	public:
		using Trainer::Trainer;
		ValidatingTrainer()
		{//ȷ��Scores������һ��TrainingScore
			Scores.resize(1);
		}

		using Trainer::Train;

		//����instances���벻ʹ��const ����ע��trainerҪSetNormalizeCopy ʵ�ʻ��Ǳ�֤���ı���������
		virtual void Train(Instances& instances, vector<Instances>& validationInstances, vector<EvaluatorPtr>& evaluators)
		{
			_instances = &instances;
			Trainer::SetNormalizeCopy(false);
			PVAL2(validationInstances.size(), _validationSets.size());
			_validationSets = move(validationInstances);
			PVAL2(validationInstances.size(), _validationSets.size());
			_evaluators = move(evaluators);
			_validating = true;
			//_valdationSetNames = from(_validationSets) >> select([](const Instances& a) { return a.name; }) >> to_vector();

			for (size_t i = 0; i < _validationSets.size(); i++)
			{
				_valdationSetNames.push_back(format("test[{}]", i));
			}

			if (_selfEvaluate)
			{
				_valdationSetNames.push_back("train");
				_validationSets.push_back(instances);
			}

			if (_selfEvaluate2)
			{
				_valdationSetNames.push_back("itrain"); //inner-train
			}

			Scores.resize(_validationSets.size());
			for (size_t i = 0; i < _validationSets.size(); i++)
			{
				Scores[i].resize(_validationSets[i].size(), 0.0);
			}

			TrainScores.resize(instances.size(), 0); //���gbdt bagging sample with fraction then will modify this size

			if (GetPredictionKind() == PredictionKind::BinaryClassification)
			{
				bool needProb = false;
				for (auto& evaluator : _evaluators)
				{
					if (evaluator->UseProbability())
					{
						needProb = true;
					}
				}
				if (needProb)
				{
					Probabilities.resize(_validationSets.size());
					for (size_t i = 0; i < _validationSets.size(); i++)
					{
						Probabilities[i].resize(_validationSets[i].size(), 0.5);
					}
				}
			}

			_evaluateResults.resize(_evaluators.size());
			for (size_t i = 0; i < _evaluators.size(); i++)
			{
				_evaluateResults[i].resize(_valdationSetNames.size(), 0);
			}

			if (_earlyStop)
			{
				InitEarlyStop();
			}
			//Trainer::Train(instances);  //����Trainer::����ʾ�Ҳ������� ע��Ҫ������ʹ��using Trainer::Train ��������Trainer::����ɶ�̬ʧЧ
			Train(instances);
		}

		virtual void Train(Instances& instances, vector<Instances>&& validationInstances, vector<EvaluatorPtr>&& evaluators)
		{
			PVAL2(validationInstances.size(), _validationSets.size());
			vector<Instances> _validationSets = move(validationInstances);
			vector<EvaluatorPtr> _evaluators = move(evaluators);
			PVAL2(validationInstances.size(), _validationSets.size());
			return Train(instances, _validationSets, _evaluators);
		}

		int BestIteration() const
		{
			return _bestCheckIteration * _earlyStopCheckFrequency;
		}

		bool Validating() const
		{
			return _validating;
		}

	protected:

		//�̳��� ÿ�ֵ�����Ҫ����,round��1��ʼ, forceEvaluate�������һ�� ǿ��Evaluate��Ҫ����trainer���������Ϣ
		//����ֵ��ʾ�Ƿ�earlyStop���Ϊtrue
		bool Evaluate(int round, bool forceEvaluate = false)
		{
			if (_validating)
			{
				_round = round;
				bool earlyStopCheck = _earlyStop && round % _earlyStopCheckFrequency == 0;
				bool evaluateCheck = round % _evaluateFrequency == 0 || forceEvaluate;
				if (earlyStopCheck)
				{
					EvaluateOnce();
					if (CheckEarlyStop())
					{
						PrintEvaluateResult(round);
						VLOG(0) << "Early stop at check round: " << _checkCounts
							<< " stop iteration: " << round
							<< " best iteration: " << _bestCheckIteration * _earlyStopCheckFrequency
							<< " best score: " << _bestScore;
						return true;
					}
					if (evaluateCheck)
					{
						PrintEvaluateResult(round);
					}
				}
				else if (evaluateCheck)
				{
					EvaluateOnce();
					PrintEvaluateResult(round);
				}
				return false;
			}
			return false;
		}

		void EvaluateOnce()
		{
			GenPredicts();
			GenProabilites();
			EvaluatePredicts();
		}
		//FastRank����ScoreTracker�д���,û����������ScoreTracker�������TrackedScore����Scores ���Կռ���
		//Or UpdateScores
		virtual void GenPredicts()
		{
			//---below for one test check for same Insatnce DataSet
			//static int round = 0;
			//for (size_t i = 0; i < Scores[0].size(); i++)
			//{
			//	CHECK_EQ(Scores[0][i], Scores[1][i]) << "round:" << round << " instance:" << i;
			//}
			//round++;
		}

		template<typename Func>
		void DoGenPredicts(Func func)
		{
			for (size_t i = 0; i < Scores.size(); i++)
			{
				for (size_t j = 0; j < Scores[i].size(); j++)
				{
					Scores[i][j] = func(_validationSets[i][j]);
				}
			}
		}

		template<typename Func>
		void DoAddPredicts(Func func)
		{
			for (size_t i = 0; i < Scores.size(); i++)
			{
				for (size_t j = 0; j < Scores[i].size(); j++)
				{
					Scores[i][j] += func(_validationSets[i][j]);
				}
			}
		}

		template<typename Func>
		void DoPredicts(Func func)
		{
			for (size_t i = 0; i < Scores.size(); i++)
			{
				for (size_t j = 0; j < Scores[i].size(); j++)
				{
					func(ref(Scores[i][j]), _validationSets[i][j]);
				}
			}
		}

		virtual void GenProabilites()
		{
			if (!Probabilities.empty())
			{
				for (size_t i = 0; i < Scores.size(); i++)
				{
					for (size_t j = 0; j < Scores[i].size(); j++)
					{//_calibrator ��ʵ�� null�� train��iter������ û��calibrate ���ͳһ���� ���� ���ܺ�test��ֵ���ڲ�һ��
						Probabilities[i][j] = _calibrator == nullptr ? gezi::sigmoid(Scores[i][j] / _scale) :
							_calibrator->PredictProbability(Scores[i][j] / _scale);
					}
				}
				TrainProbabilities.resize(TrainScores.size(), 0.5);
				for (size_t i = 0; i < TrainScores.size(); i++)
				{
					TrainProbabilities[i] = _calibrator == nullptr ? gezi::sigmoid(TrainScores[i] / _scale) :
						_calibrator->PredictProbability(TrainScores[i] / _scale);
				}
			}
		}

		virtual void EvaluatePredicts()
		{
			Fvec scaledScores;
			for (size_t i = 0; i < _evaluators.size(); i++)
			{
				for (size_t j = 0; j < _validationSets.size(); j++)
				{
					Fvec* pscore = &Scores[j];
					if (_scale != 1.0)
					{
						scaledScores = Scores[j];
						for (size_t k = 0; k < Scores[j].size(); k++)
						{
							scaledScores[k] /= _scale;
						}
						pscore = &scaledScores;
					}
					_evaluateResults[i][j] =
						Probabilities.empty() || !_evaluators[i]->UseProbability() ?
						//_evaluators[i]->Evaluate(Scores[j], _validationSets[j]) :
						_evaluators[i]->Evaluate(*pscore, _validationSets[j]) :
						_evaluators[i]->Evaluate(Probabilities[j], _validationSets[j]);
				}
				if (_selfEvaluate2)
				{
					Fvec* pscore = &TrainScores;
					if (_scale != 1.0)
					{
						scaledScores = TrainScores;
						for (size_t k = 0; k < TrainScores.size(); k++)
						{
							scaledScores[k] /= _scale;
						}
						pscore = &scaledScores;
					}
					_evaluateResults[i].back() =
						TrainProbabilities.empty() || !_evaluators[i]->UseProbability() ?
						//_evaluators[i]->Evaluate(TrainScores, *_instances) :
						_evaluators[i]->Evaluate(*pscore, *_instances) :
						_evaluators[i]->Evaluate(TrainProbabilities, *_instances);
				}
			}
		}

		virtual void StoreBestStage()
		{

		}

		virtual void RestoreBestStage()
		{

		}

	private:
		void PrintEvaluateResult(int round)
		{
			static int evluateRound = 1;
			for (size_t i = 0; i < _evaluators.size(); i++)
			{
				std::cerr << std::setiosflags(ios::left) << std::setfill(' ') << std::setw(30)
					<< format("[{}|{}]{}", evluateRound, round, _evaluators[i]->Name())
					<< gezi::print2str_row(_valdationSetNames, _evaluateResults[i], 13)
					<< std::endl;
			}
			evluateRound++;
		}

		void InitEarlyStop()
		{
			if (_evaluators[0]->LowerIsBetter())
			{
				_bestScore = std::numeric_limits<Float>::max();
			}
			else
			{
				_bestScore = std::numeric_limits<Float>::lowest();
			}
		}
	
		bool IsBetter(Float now, Float before)
		{
			return _evaluators[0]->LowerIsBetter() ? now < before :
				now > before;
		}

		bool CheckEarlyStop()
		{
			_checkCounts++;
			if (IsBetter(_evaluateResults[0][0], _bestScore))
			{
				if (_useBestStage)
				{
					StoreBestStage();
				}
				_bestScore = _evaluateResults[0][0];
				_bestCheckIteration = _checkCounts;
			}
			else
			{
				if (_checkCounts - _bestCheckIteration >= _stopRounds)
				{
					if (_useBestStage)
					{
						RestoreBestStage();
					}
					return true;
				}
			}
			return false;
		}
	public:
		ValidatingTrainer& SetEvaluateFrequency(int freq)
		{
			_evaluateFrequency = freq;
			return *this;
		}

		ValidatingTrainer& SetSelfEvaluate(bool evaluate)
		{
			_selfEvaluate = evaluate;
			return *this;
		}

		ValidatingTrainer& SetSelfEvaluate2(bool evaluate)
		{
			_selfEvaluate2 = evaluate;
			return *this;
		}

		void SetScale(double scale)
		{
			_scale = scale;
		}

		void SetSelfEvaluateInstances(const Instances& instances)
		{
			_instances = &instances;
		}

		ValidatingTrainer& SetEarlyStop(bool earlyStop = true)
		{
			_earlyStop = earlyStop;
			return *this;
		}

		ValidatingTrainer& SetEarlyStopCheckFrequency(int earlyStopCheckFrequency)
		{
			_earlyStopCheckFrequency = earlyStopCheckFrequency;
			return *this;
		}

		ValidatingTrainer& SetEarlyStopRounds(int stopRounds)
		{
			_stopRounds = stopRounds;
			return *this;
		}

		ValidatingTrainer& SetUseBestStage(bool useBestStage = true)
		{
			_useBestStage = useBestStage;
			return *this;
		}

	public:
		vector<Fvec> Scores;    //��gbdt�������͵ĵ�����Scores�����ű������н�������ۼӵ����� ���ô��ݸ�ScoreTracker����
		vector<Fvec> Probabilities;

		Fvec TrainScores;        //��ʵ���ڲ�ѵ������ʵ��score
		Fvec TrainProbabilities;
	protected:
		//-----validating args
		bool _validating = false;
		bool _selfEvaluate = false; //the same as loading the training file again but only need loading once
		bool _selfEvaluate2 = false; //truly internal self invaluate
		int _evaluateFrequency = 1; //ÿ_testFrequency�� ��һ��eval,��չʾ���

		vector<Fvec> _evaluateResults; //ÿ��evaluator��Ӧһ��vec  ������ _numValidations
		vector<Instances> _validationSets;
		vector<EvaluatorPtr> _evaluators;
		vector<string> _valdationSetNames;

		Float _scale = 1.0; //Ŀǰ��Ҫ��gbdt��bagging��ʱ�� Ϊ�˹��̵�output��������ʵ

		//-----------------------early stop
		//----early stop args
		bool _earlyStop = false;
		int _earlyStopCheckFrequency = 1; //gbdtӦ��ÿ�ֶ�check,���Ƕ�Ӧ����linearSVMû�б�Ҫ ����100�ֲ���һ��
		//���_stopRounds�ֱ���û������ ��ôstop ���������ָ��earlyStop��check����,ʵ���㷨����������* _earlyStopCheckFrequency
		//���� �����ǰ�ֺ���һ�����û�л��� ��stopRound����Ϊ1 ��ô������ֹͣ��
		int _stopRounds = 100;
		bool _useBestStage = false;

		int _bestCheckIteration = 0;
		int _roundsAfterBestIteration = 0;
		int _checkCounts = 0;
		Float _bestScore = 0;

		int _round;
	};

}  //----end of namespace gezi

#endif  //----end of M_L_CORE__TRAINER_H_
