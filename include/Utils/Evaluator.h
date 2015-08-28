/**
 *  ==============================================================================
 *
 *          \file   Utils/Evaluator.h
 *
 *        \author   chenghuige
 *
 *          \date   2015-05-22 16:36:39.427367
 *
 *  \Description: MELT��Tester��Evaluator���� Tester�������Melt��ܶ��ƻ���д�ļ� дĬ�ϵ�һЩtest�������
 *                ��ҪΪ����ʾ���㣬Evauatorÿ��ֻ����һ�����������������ϸ��� ��д�ļ�
 *  ==============================================================================
 */

#ifndef UTILS__EVALUATOR_H_
#define UTILS__EVALUATOR_H_

#include "evaluate.h"
#include "Prediction/Instances/Instance.h"
namespace gezi {
	namespace ev = evaluate;
	//ע��Ĭ��prediction���ƶ���������Output��probabilty��һ�µ� ȡ����probability,���Evaluator��ʾNotUseProbability��ôʹ��Output
	//��Ҫ֧��ʹ��probability�ģ�ʹ��Output��ֻ��Ϊʵ��
	class Evaluator
	{
	public:
		virtual string Name() const = 0;
		virtual bool LowerIsBetter() const
		{
			return true;
		}
		virtual bool UseProbability() const
		{
			return true;
		}
		//--��Ҫ�ӿ�
		//virtual Float Evaluate(const vector<Float>& predictions, const Instances& instances)
		//{
		//	return 0;
		//}

		virtual Float Evaluate(const vector<Float>& predictions, const vector<InstancePtr>& instances)
		{
			return 0;
		}

		virtual Float Evaluate(const vector<Float>& predictions, const vector<Float>& probabilities, const vector<InstancePtr>& instances)
		{
			return UseProbability() ? Evaluate(probabilities, instances) : Evaluate(predictions, instances);
		}

		virtual Float Evaluate(const vector<Float>& predictions, const vector<Float>& labels, const vector<Float>& weights)
		{
			if (weights.empty())
			{
				weights.resize(predictions.size(), 1.0);
			}
			vector<EvaluateInstancePtr> instances;
			for (size_t i = 0; i < predictions.size(); i++)
			{
				instances.push_back(make_shared<EvaluateInstance>(labels[i], weights[i]));
			}
			return Evaluate(predictions, instances);
		}

		//------����ά��
		virtual Float Evaluate(vector<EvaluateNode>& results)
		{
			vector<Float> predictions;
			vector<EvaluateInstancePtr> instances;
			for (auto& item : results)
			{
				predictions.push_back(item.prediction);
				instances.push_back(make_shared<EvaluateInstance>(item.label, item.weight));
			}
			return Evaluate(predictions, instances);
		}

		//--------below for streaming evaluator now only used in ClassiferTester.h 
		void Add(Float label, Float prediction, Float weight = 1.0)
		{
			_predictions.push_back(prediction);
			_instances.push_back(make_shared<EvaluateInstance>(label, weight));
		}
		virtual Float Finalize()
		{
			Float score = Evaluate();
			Clear();
			return score;
		}
	protected:
		void Clear()
		{
			_instances.clear();
			_predictions.clear();
		}

		Float Evaluate()
		{
			return Evaluate(_predictions, _instances);
		}

		virtual Float Evaluate(const vector<Float>& predictions, const vector<EvaluateInstancePtr>& instances)
		{
			return 0;
		}
	protected:
		vector<Float> _predictions;
		vector<shared_ptr<EvaluateInstance> > _instances;
	};

	typedef shared_ptr<Evaluator> EvaluatorPtr;

	template<typename Derived>
	class EvaluatorImpl : public Evaluator
	{
	public:
		virtual string Name() const = 0;
		virtual bool LowerIsBetter() const
		{
			return true;
		}

		//virtual Float Evaluate(const vector<Float>& predictions, const Instances& instances)
		//{
		//	return Derived::EvaluateInstances(predictions, instances);
		//}

		virtual Float Evaluate(const vector<Float>& predictions, const vector<InstancePtr>& instances)
		{
			return Derived::EvaluateInstances(predictions, instances);
		}

	protected:
		virtual Float Evaluate(const vector<Float>& predictions, const vector<EvaluateInstancePtr>& instances)
		{
			return Derived::EvaluateInstances(predictions, instances);
		}
	};

	template<typename Derived>
	class OuputEvaluatorImpl : public EvaluatorImpl < Derived >
	{
	public:
		virtual bool UseProbability() const override
		{
			return false;
		}
	};

	//auc can be used for binary classification or ranking
	class AucEvaluator : public OuputEvaluatorImpl<AucEvaluator>
	{
	public:
		virtual string Name() const
		{
			return "AUC";
		}
		virtual bool LowerIsBetter() const override
		{
			return false;
		}
		
		//--����ά��
		virtual Float Evaluate(vector<EvaluateNode>& results) override
		{
			return ev::auc(results);
		}

		template<typename Vec>
		static Float EvaluateInstances(const vector<Float>& predictions, const Vec& instances)
		{
			return ev::auc(predictions, instances);
		}
	};

	class L1Evaluator : public EvaluatorImpl<L1Evaluator>
	{
	public:
		virtual string Name() const
		{
			return "L1";
		}
		virtual Float Evaluate(vector<EvaluateNode>& results) override
		{
			return ev::l1(results);
		}

		template<typename Vec>
		static Float EvaluateInstances(const vector<Float>& predictions, const Vec& instances)
		{
			return ev::l1(predictions, instances);
		}
	};

	//squared error
	class L2Evaluator : public EvaluatorImpl<L2Evaluator>
	{
	public:
		virtual string Name() const
		{
			return "L2";
		}
		virtual Float Evaluate(vector<EvaluateNode>& results) override
		{
			return ev::l2(results);
		}

		template<typename Vec>
		static Float EvaluateInstances(const vector<Float>& predictions, const Vec& instances)
		{
			return ev::l2(predictions, instances);
		}
	};

	class RMSEEvaluator : public EvaluatorImpl<RMSEEvaluator>
	{
	public:
		virtual string Name() const
		{
			return "RMS";
		}
		virtual Float Evaluate(vector<EvaluateNode>& results) override
		{
			return ev::rmse(results);
		}

		template<typename Vec>
		static Float EvaluateInstances(const vector<Float>& predictions, const Vec& instances)
		{
			return ev::rmse(predictions, instances);
		}
	};

	class GoldStandardEvaluator : public EvaluatorImpl < GoldStandardEvaluator >
	{
	public:
		virtual string Name() const
		{
			return "GoldProb";
		}

		template<typename Vec>
		static Float EvaluateInstances(const vector<Float>& predictions, const Vec& instances)
		{
			return ev::gold_standard(predictions, instances, 0.5);
		}
	};

	class GoldStandardOutputEvaluator : public OuputEvaluatorImpl < GoldStandardOutputEvaluator >
	{
	public:
		virtual string Name() const
		{
			return "Gold";
		}

		template<typename Vec>
		static Float EvaluateInstances(const vector<Float>& predictions, const Vec& instances)
		{
			return ev::gold_standard(predictions, instances, 0);
		}
	};

	class LogLossEvaluator : public EvaluatorImpl < LogLossEvaluator >
	{
	public:
		virtual string Name() const
		{
			return "LogLossProb";
		}

		template<typename Vec>
		static Float EvaluateInstances(const vector<Float>& predictions, const Vec& instances)
		{
			return ev::logloss(predictions, instances, 30);
		}
	};

	//@TODO����fastrank�����������Ż�Ŀ���Ƶ��Ͽ�LogLoss by output�� LogLoss by probӦ��һ�� ��ʵ����Ȼ��һ�� anyway ���ֽ��;ͺá���
	class LogLossOutputEvaluator : public OuputEvaluatorImpl < LogLossOutputEvaluator >
	{
	public:
		virtual string Name() const
		{
			return "LogLoss";
		}

		template<typename Vec>
		static Float EvaluateInstances(const vector<Float>& predictions, const Vec& instances)
		{
			return ev::logloss_output(predictions, instances, 2.0);
			//return ev::logloss_output(predictions, instances, 1.0);
		}
	};

	class HingeLossEvaluator : public OuputEvaluatorImpl < HingeLossEvaluator >
	{
	public:
		virtual string Name() const
		{
			return "Hinge";
		}

		template<typename Vec>
		static Float EvaluateInstances(const vector<Float>& predictions, const Vec& instances)
		{
			return ev::hinge(predictions, instances, 1.0);
		}
	};

	class ExpLossEvaluator : public OuputEvaluatorImpl < ExpLossEvaluator >
	{
	public:
		virtual string Name() const
		{
			return "exp";
		}

		template<typename Vec>
		static Float EvaluateInstances(const vector<Float>& predictions, const Vec& instances)
		{
			return ev::exploss(predictions, instances, 1.0);
		}
	};
}  //----end of namespace gezi

#endif  //----end of UTILS__EVALUATOR_H_
