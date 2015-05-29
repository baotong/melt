/**
 *  ==============================================================================
 *
 *          \file   Utils/evaluate.h
 *
 *        \author   chenghuige
 *
 *          \date   2015-05-26 12:54:24.419936
 *
 *  \Description:
 *  ==============================================================================
 */

#ifndef UTILS_EVALUATE_H_
#define UTILS_EVALUATE_H_

#include "evaluate_def.h"
#include "statistic_util.h"
namespace gezi {

	//���������һ������������һ����������������������score�ж��ĸ��ʴ��ڸ���������score������������壬���Ǿ͵õ�������һ�м���AUC�İ취���õ�������ʡ�����֪�������������������ǳ��õĵõ����ʵİ취����ͨ��Ƶ��������֮�����ֹ�������������ģ��������𽥱ƽ���ʵֵ���� ������ķ����У�������Խ�࣬�����AUCԽ׼ȷ���ƣ�Ҳ�ͼ�����ֵ�ʱ��С���仮�ֵ�Խϸ�������Խ׼ȷ��ͬ���ĵ���������˵����ͳ��һ�����е� M��N(MΪ������������Ŀ��NΪ������������Ŀ)�������������У��ж��ٸ����е���������score���ڸ�������score������Ԫ�������������� score��ȵ�ʱ�򣬰���0.5���㡣Ȼ�����MN��ʵ����������ĸ��Ӷ�ΪO(n ^ 2)��nΪ����������n = M + N��
	//	�����ַ���ʵ���Ϻ������ڶ��ַ�����һ���ģ����Ǹ��Ӷȼ�С�ˡ���Ҳ�����ȶ�score�Ӵ�С����Ȼ�������score��Ӧ��sample ��rankΪn���ڶ���score��Ӧsample��rankΪn - 1���Դ����ơ�Ȼ������е�����������rank��ӣ��ټ�ȥ����������scoreΪ�� С����M��ֵ��������õ��ľ������е��������ж��ٶ�����������score���ڸ���������score��Ȼ���ٳ���M��N���� AUC=((���е�����λ�����)-M*(M+1))/(M*N) ���⣬�ر���Ҫע����ǣ��ٴ���score��ȵ����ʱ�������score����������Ҫ ������ͬ��rank(���������ȵ�score�ǳ�����ͬ���������ǲ�ͬ�������֮�䣬����Ҫ��������)��������������ٰ�������Щscore��ȵ����� ��rankȡƽ����Ȼ����ʹ��������ʽ��
	//http://www.tuicool.com/articles/qYNNF3
	/*2. ������M����������N���������棬�ܹ������M*N�������ԣ�ͳ��һ������Щ���������ж�����������score���ڸ�������score��������K�ԣ���ôAUC��ֵ����K / (M*N)
	����˵��һ�£�
	������y = 1��y = 1�� y = 1�� y = -1�� y = -1�� y = -1
	ģ��1��Ԥ�⣺0.8��0.7��0.3��0.5��0.6��0.9
	ģ��2��Ԥ�⣺0.1�� 0.8�� 0.9�� 0.5�� 0.85�� 0.2
	ģ��1��������score���ڸ����Ķ԰���(y1, y4)(y1, y5)(y2, y4)(y2, y5)������AUCֵΪ4 / 9
	ģ��2��������score���ڸ������Ķ԰���(y2, y4)(y2, y6)(y3, y4)(y3, y5)(y3, y6)������AUC��ֵΪ5 / 9
	����ģ��2Ҫ��ģ��1��
	�����㷨�ĸ��Ӷ�ΪO(n ^ 2)����n = (M + N)Ҳ��������������
	3. ����3������2��һ���ģ�ֻ��������һЩ�����С�˸��Ӷȣ����Ȱ���score�������򣬵÷�����Ϊn���ڶ����Ϊn - 1���������ƣ���Сһ����Ϊ1����ôAUC�ļ��㷽��Ϊ��AUC = ((������������֮��)-m*(m + 1) / 2) / (M*N)��
	����ʽ�е��������������ӽ���һ��
	ģ��1�����ȶ�Ԥ���score������������������Ϊ������6��������5��������4��������3��������2��������1��
	AUC��ֵΪ��(��5 + 4 + 1�� - 3 * ��3 + 1�� / 2) / (3 * 3) = 4 / 9�����Կ������������ļ�����һ�£����ǿ�һ��������㹫ʽ�����ȷ����Ϻ���Ĳ���M*��M + 1�� / 2���ǲ��Ǻ���Ϥ��Сѧ��֪�����ϵ׼��µ�������������2�����������ε������ʽ��Ҳ��������ֵ�Ĺ�ʽ������1 + 2 + 3 + 4��������ָ�ľ������е��������ĵ÷ֶ�С�����еĸ������ĵ÷ֵ�����£����������ֵ��ǰ�벿��ָ����ʵ�ʵ������������������Ӧ�ñȽϺ�����˰�*/
	namespace evaluate
	{
		template<typename Vec>
		inline Float auc(const vector<Float>& predictions, const Vec& instances, bool needSort = true)
		{
			if (predictions.empty())
			{
				return 0.5;
			}

			vector<int> indexes(predictions.size());
			for (size_t i = 0; i < predictions.size(); i++)
			{
				indexes[i] = i;
			}
			if (needSort)
			{
				gezi::sort(predictions, indexes, gezi::IndexCmper<>());
			}

			Float oldFalsePos = 0;
			Float oldTruePos = 0;
			Float falsePos = 0;
			Float truePos = 0;
			Float oldOut = std::numeric_limits<Float>::infinity();
			Float result = 0;

			for (size_t i = 0; i < indexes.size(); i++)
			{
				int index = indexes[i];
				Float label = instances[index]->label;
				Float prediction = predictions[index];
				Float weight = instances[index]->weight;
				//Pval3(label, output, weight);
				if (prediction != oldOut) //������ֵͬ����������⴦���
				{
					result += 0.5 * (oldTruePos + truePos) * (falsePos - oldFalsePos);
					//Pval((0.5 * (oldTruePos + truePos) * (falsePos - oldFalsePos)));
					//Pval4(oldTruePos, truePos, oldFalsePos, falsePos);
					oldOut = prediction;
					oldFalsePos = falsePos;
					oldTruePos = truePos;
				}
				if (label > 0)
					truePos += weight;
				else
					falsePos += weight;
			}
			//Pval((0.5 * (oldTruePos + truePos) * (falsePos - oldFalsePos)));
			result += 0.5 * (oldTruePos + truePos) * (falsePos - oldFalsePos);
			Float AUC = result / (truePos * falsePos);
			return AUC;
		}

		template<typename Vec>
		inline Float l1(const vector<Float>& predictions, const Vec& instances)
		{
			if (predictions.empty())
			{
				return 0;
			}
			Float error = 0;
			for (size_t i = 0; i < predictions.size(); i++)
			{
				error += std::abs(predictions[i] - instances[i]->label) * instances[i]->weight;
			}
			return error / predictions.size();
		}

		template<typename Vec>
		inline Float l2(const vector<Float>& predictions, const Vec& instances)
		{
			if (predictions.empty())
			{
				return 0;
			}
			Float error = 0;
			for (size_t i = 0; i < predictions.size(); i++)
			{
				Float diff = predictions[i] - instances[i]->label;
				error += diff * diff * instances[i]->weight;
			}
			return error / predictions.size();
		}

		template<typename Vec>
		inline Float rmse(const vector<Float>& predictions, const Vec& instances)
		{
			return std::sqrt(l2(predictions, instances));
		}

		template<typename Vec>
		inline Float logloss(const vector<Float>& predictions, const Vec& instances, Float logTolerence = 30)
		{
			if (predictions.empty())
			{
				return 0;
			}
			Float error = 0;
			for (size_t i = 0; i < predictions.size(); i++)
			{
				Float trueProb = (instances[i]->label > 0 ? 1 : 0);
				error += gezi::cross_entropy_toleranced(trueProb, predictions[i], logTolerence) * instances[i]->weight; //ʹ��ln ��ʹ��log2
			}
			return error / predictions.size();
		}

		//������ͨ��output�����LogLoss output ��ͨ��probability�����LogLoss ProbӦ��һ�� ���ǿ��ǵ�������� �Ƚϴ��ouput ������prob����Ϊ1 ����log����Ϊinf 
		//@TODO ����ȥʹ��output�������
		//����log�����������tolerence����Ҳ���ǿ���logloss ouput��logloss prob��΢С�Ĳ�һ��
		//W0528 14:54:09.922850 24084 ClassifierTester.h:109] Bad predict label:0 prediction:19.0708 probability:1 currLogLossProb:inf
		template<typename Vec>
		inline Float logloss_output(const vector<Float>& predictions, const Vec& instances, Float beta = 2.0)
		{
			if (predictions.empty())
			{
				return 0;
			}
			Float error = 0;
			for (size_t i = 0; i < predictions.size(); i++)
			{
				Float trueOutput = (instances[i]->label > 0 ? 1 : -1);
				error += loss::logistic(trueOutput, predictions[i], beta) * instances[i]->weight;
			}
			return error / predictions.size();
		}

		//������ouput �� logloss_outputʵ�ʲ��
		template<typename Vec>
		inline Float exploss(const vector<Float>& predictions, const Vec& instances, Float beta = 1.0)
		{
			if (predictions.empty())
			{
				return 0;
			}
			Float error = 0;
			for (size_t i = 0; i < predictions.size(); i++)
			{
				Float trueOutput = (instances[i]->label > 0 ? 1 : -1);
				error += std::exp(-beta * trueOutput * predictions[i]) * instances[i]->weight;
			}
			return error / predictions.size();
		}

		//thre = 0.5��ʾ���ո���, 0 ��ʾ����margin output
		template<typename Vec>
		inline Float gold_standard(const vector<Float>& predictions, const Vec& instances, Float thre = 0.5)
		{
			if (predictions.empty())
			{
				return 0;
			}
			Float error = 0;
			for (size_t i = 0; i < predictions.size(); i++)
			{
				Float trueProb = (instances[i]->label > 0 ? 1 : 0);
				error += (predictions[i] > thre ? 1 - trueProb : trueProb) * instances[i]->weight;
			}
			return error / predictions.size();
		}

		//ֻ֧��margin output��Ϊ����
		template<typename Vec>
		inline Float hinge(const vector<Float>& predictions, const Vec& instances, Float margin = 1.0)
		{
			if (predictions.empty())
			{
				return 0;
			}
			Float error = 0;
			for (size_t i = 0; i < predictions.size(); i++)
			{
				Float trueOutput = (instances[i]->label > 0 ? 1 : -1);
				Float distance = predictions[i] * trueOutput;
				if (distance <= margin)
				{
					error += (margin - distance) * instances[i]->weight;
				}
			}
			return error / predictions.size();
		}

		//----------------------------- below is depreciated ����ά�� ��Ҫ��ʹ��EvaluateNodeֻ��չʾ����һ�ֿ��ܵĽӿ���� 
		//Ҳ����ʹ��3��vector labels, predictions, weights�����
		//need label, prediction and weight
		inline Float auc(const vector<Node>& results, bool needSort = true)
		{
			if (results.empty())
			{
				return 0.5;
			}
			if (needSort)
			{
				stable_sort(results.begin(), results.end(),
					[](const Node& l, const Node& r) {
					return l.prediction > r.prediction; });
			}

			Float oldFalsePos = 0;
			Float oldTruePos = 0;
			Float falsePos = 0;
			Float truePos = 0;
			Float oldOut = std::numeric_limits<Float>::infinity();
			Float result = 0;

			for (auto& item : results)
			{
				Float label = item.label;
				Float prediction = item.prediction;
				Float weight = item.weight;
				//Pval3(label, output, weight);
				if (prediction != oldOut) //������ֵͬ����������⴦���
				{
					result += 0.5 * (oldTruePos + truePos) * (falsePos - oldFalsePos);
					//Pval((0.5 * (oldTruePos + truePos) * (falsePos - oldFalsePos)));
					//Pval4(oldTruePos, truePos, oldFalsePos, falsePos);
					oldOut = prediction;
					oldFalsePos = falsePos;
					oldTruePos = truePos;
				}
				if (label > 0)
					truePos += weight;
				else
					falsePos += weight;
			}
			//Pval((0.5 * (oldTruePos + truePos) * (falsePos - oldFalsePos)));
			result += 0.5 * (oldTruePos + truePos) * (falsePos - oldFalsePos);
			Float AUC = result / (truePos * falsePos);
			return AUC;
		}

		inline Float l1(vector<Node>& results)
		{
			if (results.empty())
			{
				return 0;
			}
			Float error = 0;
			for (size_t i = 0; i < results.size(); i++)
			{
				error += std::abs(results[i].prediction - results[i].label) * results[i].weight;
			}
			return error / results.size();
		}

		inline Float l2(vector<Node>& results)
		{
			if (results.empty())
			{
				return 0;
			}
			Float error = 0;
			for (size_t i = 0; i < results.size(); i++)
			{
				Float diff = results[i].prediction - results[i].label;
				error += diff * diff * results[i].weight;
			}
			return error / results.size();
		}

		inline Float rmse(vector<Node>& results)
		{
			return std::sqrt(l2(results));
		}

	} //---- end of namespace evaluate
}  //----end of namespace gezi

#endif  //----end of UTILS_EVALUATE_H_
