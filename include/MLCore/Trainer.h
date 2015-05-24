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

namespace gezi {

	class Trainer : public WithArgs, public WithHelp
	{
	public:
		virtual PredictionKind GetPredictionKind()
		{
			return PredictionKind::BinaryClassification;
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
		//����instances���벻ʹ��const ����ע��trainerҪSetNormalizeCopy ʵ�ʻ��Ǳ�֤���ı���������
		virtual void Train(Instances& instances, vector<Instances>& valicationInstances, vector<EvaluatorPtr>& evaluators)
		{
			Trainer::SetNormalizeCopy();
			_validationSets = move(valicationInstances);
			_evaluators = move(evaluators);
			_validating = true;
			_valdationSetNames = from(_validationSets) >> select([](const Instances& a) { return a.name; }) >> to_vector();
			Trainer::Train(instances);  //����Trainer::����ʾ�Ҳ��� ���ظ��� ����Ҳ���Ժ������������� using Trainer::Train
		}
	protected:
		bool IsValidating()
		{
			return _validating;
		}

		virtual void PrepareEvaluate()
		{
			_results.clear();
		}

		void Evaluate()
		{
			vector<Fvec> evaluateResults = GetEvaluateResults(_results);
			PrintEvaluateResult(evaluateResults);
		}

	private:
		vector<Fvec> GetEvaluateResults(vector<vector<EvaluateNode> >& results)
		{
			vector<Fvec> evaluateResults(_evaluators.size()); //numRows=numEvaluators,numCols=numValidationInstances
			for (size_t i = 0; i < _evaluators.size(); i++)
			{
				evaluateResults.resize(_validationSets.size());
			}
#pragma omp parallel for schedule(static)
			for (size_t i = 0; i < _evaluators.size(); i++)
			{
#pragma omp parallel for schedule(static)
				for (size_t j = 0; j < _validationSets.size(); j++)
				{
					evaluateResults[i][j] = _evaluators[i]->Evaluate(_results[j]);
				}
			}
			return evaluateResults;
		}

		void PrintEvaluateResult(const vector<Fvec>&  evaluateResults)
		{
			for (size_t i = 0; i < _evaluators.size(); i++)
			{
				std::cerr << "[" << i << "]" << _evaluators[i]->Name() << " ";
				gezi::print(_valdationSetNames, evaluateResults[i], ":", "\t");
				std::cerr << std::endl;
			}
		}
	protected:
		bool _validating = false;

		vector<vector<EvaluateNode> > _results; //_results.size() == numValidationInsatnces
		vector<string> _valdationSetNames;
		vector<Instances> _validationSets;
		vector<EvaluatorPtr> _evaluators;
	};

}  //----end of namespace gezi

#endif  //----end of M_L_CORE__TRAINER_H_
