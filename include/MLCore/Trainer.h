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

		//����instances���벻ʹ��const ����ע��trainerҪSetNormalizeCopy ʵ�ʻ��Ǳ�֤���ı���������
		virtual void Train(Instances& instances, vector<Instances>& validationInstances, vector<EvaluatorPtr>& evaluators)
		{
			_instances = &instances;
			Trainer::SetNormalizeCopy(false);
			_validationSets = move(validationInstances);
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
			for (int i = 0; i < _validationSets.size(); i++)
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
					for (int i = 0; i < _validationSets.size(); i++)
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

			Trainer::Train(instances);  //����Trainer::����ʾ�Ҳ��� ���ظ��� ����Ҳ���Ժ������������� using Trainer::Train
		}

		virtual void Train(Instances& instances, vector<Instances>&& validationInstances, vector<EvaluatorPtr>&& evaluators)
		{
			vector<Instances> _validationSets = move(validationInstances);
			vector<EvaluatorPtr> _evaluators = move(evaluators);
			return Train(instances, _validationSets, _evaluators);
		}
	protected:

		void Evaluate(int round)
		{
			GenPredicts();
			GenProabilites();
			EvaluatePredicts();
			PrintEvaluateResult(round);
		}

		//FastRank����ScoreTracker�д���,û����������ScoreTracker�������TrackedScore����Scores ���Կռ���
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

		//ÿ�ָ���score  Fastrank�Լ���ScoreTracker�д���,linearSvm����ҪUpdateScores ����ֱ�Ӽ���Predicts
		virtual void UpdateScores()
		{

		}

		virtual void GenProabilites()
		{
			if (!Probabilities.empty())
			{
				for (size_t i = 0; i < Scores.size(); i++)
				{
					for (size_t j = 0; j < Scores[i].size(); j++)
					{//_calibrator ��ʵ�� null�� train��iter������ û��calibrate ���ͳһ���� ���� ���ܺ�test��ֵ���ڲ�һ��
						Probabilities[i][j] = _calibrator == nullptr ? gezi::sigmoid(Scores[i][j]) :
							_calibrator->PredictProbability(Scores[i][j]);
					}
				}
				TrainProbabilities.resize(TrainScores.size(), 0.5);
				for (size_t i = 0; i < TrainScores.size(); i++)
				{
					TrainProbabilities[i] = _calibrator == nullptr ? gezi::sigmoid(TrainScores[i]) :
						_calibrator->PredictProbability(TrainScores[i]);
				}
			}
		}

		virtual void EvaluatePredicts()
		{
			for (size_t i = 0; i < _evaluators.size(); i++)
			{
				for (size_t j = 0; j < _validationSets.size(); j++)
				{
					_evaluateResults[i][j] =
						Probabilities.empty() || !_evaluators[i]->UseProbability() ?
						_evaluators[i]->Evaluate(Scores[j], _validationSets[j]) :
						_evaluators[i]->Evaluate(Probabilities[j], _validationSets[j]);
				}
				if (_selfEvaluate2)
				{
					_evaluateResults[i].back() =
						TrainProbabilities.empty() || !_evaluators[i]->UseProbability() ?
						_evaluators[i]->Evaluate(TrainScores, *_instances) :
						_evaluators[i]->Evaluate(TrainProbabilities, *_instances);
				}
			}
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

	public:
		ValidatingTrainer& SetTestFrequency(int freq)
		{
			_testFrequency = freq;
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

	public:
		vector<Fvec> Scores;    //��ʹ��_selfEaluate��ôҲ�Ǽ��������Insatnces�����
		vector<Fvec> Probabilities;

		Fvec TrainScores;        //��ʵ���ڲ�ѵ������ʵ��score
		Fvec TrainProbabilities;
	protected:
		bool _validating = false;

		vector<Fvec> _evaluateResults; //ÿ��evaluator��Ӧһ��vec  ������ _numValidations

		vector<Instances> _validationSets;
		vector<EvaluatorPtr> _evaluators;

		vector<string> _valdationSetNames;

		bool _selfEvaluate = false; //the same as loading the training file again but only need loading once
		bool _selfEvaluate2 = false; //truly internal self invaluate
		int _testFrequency = 1;
	};

}  //----end of namespace gezi

#endif  //----end of M_L_CORE__TRAINER_H_
