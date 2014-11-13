/**
 *  ==============================================================================
 *
 *          \file   performance_evaluate.h
 *
 *        \author   chenghuige
 *
 *          \date   2014-07-25 15:07:01.534980
 *
 *  \Description:
 *  ==============================================================================
 */

#ifndef PERFORMANCE_EVALUATE_H_
#define PERFORMANCE_EVALUATE_H_

#include "common_util.h"
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
	inline Float auc(vector<std::tuple<int, Float, Float> >& results, bool needSort = true)
	{
		if (needSort)
		{
			stable_sort(results.begin(), results.end(),
				[](const std::tuple<int, Float, Float>& l, const std::tuple<int, Float, Float>& r) {
				return std::get<1>(l) > std::get<1>(r); });
		}

		Float oldFalsePos = 0;
		Float oldTruePos = 0;
		Float falsePos = 0;
		Float truePos = 0;
		Float oldOut = std::numeric_limits<Float>::infinity();
		Float result = 0;

		for (auto& item : results)
		{
			int label = std::get<0>(item);
			Float output = std::get<1>(item);
			Float weight = std::get<2>(item);
			//Pval3(label, output, weight);
			if (output != oldOut) //������ֵͬ����������⴦���
			{
				result += 0.5 * (oldTruePos + truePos) * (falsePos - oldFalsePos);
				//Pval((0.5 * (oldTruePos + truePos) * (falsePos - oldFalsePos)));
				//Pval4(oldTruePos, truePos, oldFalsePos, falsePos);
				oldOut = output;
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


	//-----------------below depreciated
	class BinaryClassificationEvaluator
	{
	public:
		virtual void Add(int label, Float output, Float weight = 1.0)
		{

		}

		virtual double Finish()
		{
			return 0;
		}
	};

	typedef shared_ptr<BinaryClassificationEvaluator> BinaryClassficationEvaluatorPtr;
	class AucEvaluator : public BinaryClassificationEvaluator
	{
	public:
		virtual void Add(int label, Float output, Float weight = 1.0) override
		{
			_results.push_back(std::make_tuple(label, output, weight));
		}

		virtual double Finish() override
		{
			return auc(_results);
		}
	private:
		vector<std::tuple<int, Float, Float> > _results;
	};

}  //----end of namespace gezi

#endif  //----end of PERFORMANCE_EVALUATE_H_
