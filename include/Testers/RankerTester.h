/**
 *  ==============================================================================
 *
 *          \file   Testers/RankerTester.h
 *
 *        \author   chenghuige
 *
 *          \date   2014-11-12 08:46:53.032820
 *
 *  \Description:
 *  ==============================================================================
 */

#ifndef TESTERS__RANKER_TESTER_H_
#define TESTERS__RANKER_TESTER_H_
#include "Tester.h"
namespace gezi {

	class RankingMetrics : public DatasetMetrics
	{
	public:
		RankingMetrics()
		{
			ParseArgs();

			sumNDCGAtN.resize(truncationLevel, 0);
			sumDCGAtN.resize(truncationLevel, 0);
			discountMap.resize(truncationLevel);
			double numerator = useLn ? 1.0 : log(2);
			for (int i = 0; i < truncationLevel; i++)
			{
				discountMap[i] = numerator / log(2 + i);
			}
		}

		virtual void ParseArgs() override;

		virtual vector<string> PerInstanceColumnNames() override
		{
			vector<string> names = { "Predicted" };
			AddPerInstanceNameGroup(names, "NDCG");
			AddPerInstanceNameGroup(names, "DCG");
			AddPerInstanceNameGroup(names, "MaxDCG");
			return names;
		}

		virtual vector<string> Names() override
		{
			vector<string> names;
			AddNameGroup(names, "NDCG");
			AddNameGroup(names, "DCG");
			names.push_back("Top1RankPrecision(avg)");  //��һλ��׼ȷ�ʣ�������ձ�������(��ͬscore,Сlabel��ǰ)������һ���� ��query�µ����label ��Ϊ׼ȷ������Ϊ��׼ȷ
			names.push_back("FilteredTop1RankPrecision(avg)"); //ͬ��,����ȥ�������label��0��query
			names.push_back("NumQuriesWithoutRelevantDocs");
			return names;
		}

		virtual Fvec ProcessInstance(Float label, Float prediction, Float probability, Float weight) override
		{
			//AutoTimer timer("ProcessInstance");

			queryLabels.push_back((short)label);
			queryOutputs.push_back(prediction);
			if (std::isnan(currentQueryWeight))
			{
				currentQueryWeight = weight;
			}
			else if (currentQueryWeight != weight)
			{
				LOG(WARNING) << "Weights within query differ";
			}
			return Fvec({ prediction });
		}

		//input labels, output maxDcG vec
		dvec QueryMaxDCG(const vector<short>& labels)
		{
			dvec maxAtN = dvec(truncationLevel);
			int relevancyLevel = gainMap.size();

			ivec labelCounts(relevancyLevel, 0);

			int labelLength = labels.size(); //doc num for this query
			int maxTrunc = std::min(truncationLevel, labelLength); //label����Ŀ�п��ܻ�� truncationLevel С,maxTrunc >= 1

			CHECK_GT(maxTrunc, 0);

			//��������   4,4,4,3,2,1,1 ���������źü������ŵ�DCG
			for (int l = 0; l < labelLength; l++)
				labelCounts[labels[l]]++;

			int topLabel = relevancyLevel - 1;
			while (labelCounts[topLabel] == 0)
				topLabel--;
			maxAtN[0] = gainMap[topLabel] * discountMap[0];
			labelCounts[topLabel]--;
			for (int t = 1; t < maxTrunc; t++)
			{
				while (labelCounts[topLabel] == 0)
					topLabel--;
				maxAtN[t] = maxAtN[t - 1] + gainMap[topLabel] * discountMap[t];
				labelCounts[topLabel]--;
			}
			for (int t = maxTrunc; t < truncationLevel; t++)
			{
				maxAtN[t] = maxAtN[t - 1];
			}

			return maxAtN;
		}


		template<typename ValueVec, typename Compare>
		void index_sort(const ValueVec& valueVec, vector<int>& indexVec, Compare compare, int maxLen)
		{
			indexVec.resize(valueVec.size());
			for (size_t i = 0; i < indexVec.size(); i++)
			{
				indexVec[i] = i;
			}
			if (maxLen >= indexVec.size())
			{
				std::sort(indexVec.begin(), indexVec.end(),
					[&valueVec, &compare](const int l, const int r) { return compare(valueVec[l], valueVec[r]); });
			}
			else
			{
				std::partial_sort(indexVec.begin(), indexVec.begin() + maxLen, indexVec.end(),
					[&valueVec, &compare](const int l, const int r) { return compare(valueVec[l], valueVec[r]); });
			}
		}

		dvec QueryDCG(const vector<short>& labels, const Fvec& scores)
		{
			dvec dcgAtN = dvec(truncationLevel);

			// calculate the permutation, @TODO may be use partial_sort to choose only first truncationLevel? TLCҲ��ȫ��sort
			int count = labels.size();

			ivec permutation = gezi::range_vec(count);
			//Pval2(scores[0], scores[1]);
			//Pval((scores[0] > scores[1]));
			//Pval((scores[0] < scores[1]));
			//Pval((scores[0] == scores[1]));
			//@TODO or use stable sort? �����TLC����һ��
			//@FIXME �������н����һ�£����� ������ͬ��feature preictorԤ����Ӧ��һ�£����� ����score�ԱȻ�������в�һ��
			//I0619 14:15 : 14.166950 12286 RankerTester.h : 144] (scores[0] > scores[1])-- - [0]
			//I0619 14:15 : 14.166965 12286 RankerTester.h : 145] (scores[0] < scores[1])-- - [0]
			//I0619 14:15 : 14.166975 12286 RankerTester.h : 146] (scores[0] == scores[1])-- - [1]
			//NDCG@1(avg)0
			//NDCG@2(avg)                             63.093
			//NDCG@3(avg)                             63.093
			//DCG@1(avg)0
			//DCG@2(avg)                              2.73072
			//DCG@3(avg)                              2.73072

			//I0619 14:15 : 15.214525 12297 RankerTester.h : 143] scores[0] --[0.939999] scores[1] --[0.939999]
			//I0619 14:15 : 15.214609 12297 RankerTester.h : 144] (scores[0] > scores[1])-- - [1]
			//I0619 14:15 : 15.214638 12297 RankerTester.h : 145] (scores[0] < scores[1])-- - [0]
			//I0619 14:15 : 15.214659 12297 RankerTester.h : 146] (scores[0] == scores[1])-- - [0]
			//NDCG@1(avg)100
			//NDCG@2(avg)100
			//NDCG@3(avg)100
			//DCG@1(avg)                              4.32809
			//DCG@2(avg)                              4.32809
			//DCG@3(avg)                              4.32809
			//һ���������Ҫ �����ж� epsilon operator > < == is ok����������Ҫ�ж�
			std::sort(permutation.begin(), permutation.end(), [&scores, &labels](const int l, const int r)
			{
				Float diff = scores[l] - scores[r];
				//const Float eps = (Float)1e-8;
				//if (diff > eps)
				//{
				//	return true;
				//}
				//else if (diff < -eps)
				//{
				//	return false;
				//}
				//if (scores[l] > scores[r])
				//--epsilon seems not work
				//if (diff > std::numeric_limits<Float>::epsilon())
				//lowest() -- 0 -- min() -- max()
				//if (std::nextafter(scores[l], std::numeric_limits<Float>::lowest()) > scores[r])
				//{
				//	//VLOG(0) << "1";
				//	return true;
				//}
				// epsilon not work diff--[-4.44089e-16] - std::numeric_limits<Float>::epsilon()--[-2.22045e-16]
				//epsilon �����������С�й�ϵ Ĭ����1
				//else if (diff < -std::numeric_limits<Float>::epsilon())
				//Epsilon is defined as "Machine epsilon (the difference between 1 and the least value greater than 1 that is representable)" - source http://www.cplusplus.com/reference/std/limits/numeric_limits/. This is approximately 2.22045e-016 (source http://msdn.microsoft.com/en-us/library/6x7575x3(v=vs.71).aspx)
				//else if (std::nextafter(scores[l], std::numeric_limits<Float>::max()) < scores[r])
				//{
				//	//VLOG(0) << "2";
				//	return false;
				//}
				if (diff == 0 ||
					FloatingPoint<Float>(scores[l]).AlmostEquals(FloatingPoint<Float>(scores[r])))
				{ //==
					//@TODO or use stable_sort and here //return labels[l] < labels[r] ?
					if (labels[l] < labels[r])
					{
						//VLOG(0) << "3";
						return true;
					}
					else if (labels[l] > labels[r])
					{
						//VLOG(0) << "4";
						return false;
					}
					else
					{
						//VLOG(0) << "5";
						return l < r;
					}
				}
				else
				{
					if (diff > 0)
					{
						return true;
					}
					else
					{
						return false;
					}
				}
			});

			//Pvec(scores);
			//Pvec(labels);
			//Pvec(permutation);

			if (count > truncationLevel)
				count = truncationLevel;
			double dcg = 0;
			for (int t = 0; t < count; ++t)
			{
				dcg = dcg + gainMap[labels[permutation[t]]] * discountMap[t];
				dcgAtN[t] = dcg;
			}
			if (dcg > 0)
			{
				for (int t = count; t < truncationLevel; ++t)
					dcgAtN[t] = dcg;
			}
			return dcgAtN;
		}

		virtual Fvec FinishGroup()
		{
			dvec maxDCGAtN = QueryMaxDCG(queryLabels);
			dvec DCGAtN = QueryDCG(queryLabels, queryOutputs);
			dvec NDCGAtN = dvec(truncationLevel);

			for (int t = 0; t < truncationLevel; t++)
			{ //@CHECK TLC ��   Double ndcg = DCGAtN[t] > 0 ? DCGAtN[t] / maxDCGAtN[t] * 100 : 0; ���˰ɡ���
				double ndcg = maxDCGAtN[t] > 0 ? DCGAtN[t] / maxDCGAtN[t] * 100 : 0;
				sumNDCGAtN[t] += ndcg * currentQueryWeight;
				sumDCGAtN[t] += DCGAtN[t] * currentQueryWeight;
				NDCGAtN[t] = ndcg;
			}

			if (maxDCGAtN[0] > 0)
			{
				sumWeights2 += currentQueryWeight;
				if (NDCGAtN[0] == 100)
				{
					sumTop1Precision += 1.0;
					sumTop1Precision2 += 1.0;
				}
			}
			else
			{
				numNoRelevantQuries++;
				sumTop1Precision += 1.0;
			}

			sumWeights += currentQueryWeight;

			queryLabels.clear();
			queryOutputs.clear();

			Fvec results;
			//for (auto val : NDCGAtN)
			//{
			//	results.push_back(val);
			//}
			//for (auto val : DCGAtN)
			//{
			//	results.push_back(val);
			//}
			//for (auto val : maxDCGAtN)
			//{
			//	results.push_back(val);
			//}
			////@TODO @FIXME
			gezi::append(results, NDCGAtN);
			gezi::append(results, DCGAtN);
			gezi::append(results, maxDCGAtN);

			return results;
		}

		virtual void Finish() override
		{
			_results.resize(sumNDCGAtN.size() + sumDCGAtN.size() + 3, std::numeric_limits<double>::quiet_NaN());
			Float denominator = excludeNoRelevant ? sumWeights2 : sumWeights;
			int index = 0;
			if (denominator > 0)
			{
				for (auto value : sumNDCGAtN)
				{
					_results[index++] = value / denominator;
				}
				for (auto value : sumDCGAtN)
				{
					_results[index++] = value / denominator;
				}
			}

			index = sumNDCGAtN.size() + sumDCGAtN.size();
			if (sumWeights > 0)
			{
				_results[index++] = sumTop1Precision / sumWeights;
			}

			if (sumWeights2 > 0)
			{
				_results[index++] = sumTop1Precision2 / sumWeights2;
			}

			_results[index] = numNoRelevantQuries;
		}

	private:
		void AddPerInstanceNameGroup(vector<string>& names, string prefix)
		{
			for (int i = 0; i < truncationLevel; i++)
			{
				names.push_back(format("{}@{}", prefix, i + 1));
			}
		}

		void AddNameGroup(vector<string>& names, string prefix)
		{
			for (int i = 0; i < truncationLevel; i++)
			{
				names.push_back(format("{}@{}(avg)", prefix, i + 1));
			}
		}
	protected:
		int truncationLevel = 3;
		//string labelGains = "0,3,7,15,31"; //΢��ı�׼,һ��������2^label - 1,Ҳ���� 0, 1, 3, 7, 15 ...
		bool useLn = false;
		bool excludeNoRelevant = false;

		Fvec gainMap;
		dvec discountMap;

		vector<short> queryLabels;
		Fvec queryOutputs;

		//Float topQueryOutpus = -1;  //������һ��output/predictionֵ
		//Float topQueryLabel = -1;  //������һ����doc��Ӧ��label��ע�����top1,top2 label��ͬ ��ôlabelС�ķ���ǰ�� ���۹���
		//Float maxQueryLabels = 0;  //query��������/��label

		Float currentQueryWeight = std::numeric_limits<Float>::quiet_NaN();

		Float sumWeights = 0;
		Float sumWeights2 = 0; //ȥ����badquery
		int numNoRelevantQuries = 0; //ͳ��û������ĵ���query��Ŀ ������doc��Ӧlabel ��Ϊ0

		//static const int maxTruncationLevel = 100;  //use fix sized array to speed up maybe
		Fvec sumDCGAtN;
		Fvec sumNDCGAtN;
		Float sumTop1Precision = 0; //����top1������׼ȷ�ʣ�ֻҪ���label����ǰ�����׼ȷ��bad query��ӦҲ��׼ȷ
		Float sumTop1Precision2 = 0; //ͬ��,���ǲ�����bad query
	};

	class RankerTester : public Tester
	{
	public:
		RankerTester()
		{
			hasRankingMetric = true;
		}

		virtual PredictionKind GetPredictionKind() override
		{
			return PredictionKind::Ranking;
		}

		virtual vector<DatasetMetricsPtr> ConstructDatasetMetrics() override
		{
			return vector<DatasetMetricsPtr>({ make_shared<RankingMetrics>() });
		}

		virtual void ProcessInstances(Instances instances, PredictorPtr predictor) override
		{
			CHECK(instances.IsRankingInstances()) << "RankerTester work only for RankingInstances, with groupKey specified with -group command line";
			auto& instanceGroups = instances.GetGroups();

			AutoTimer timer("ProcessInstances");

			int64 idx = 0;
			for (auto& group : instanceGroups)
			{
				//AutoTimer timer("Deal one Group");

				string groupKey = group.first;
				auto& groupInstances = group.second;
				vector<dvec> rankingOutputs;
				//@TODO ����ÿ���鲢�д���Ҳ��
				{
					//AutoTimer timer("Deal Group instances");
					//auto predictions = predictor->BulkPredict(groupInstances);
					//size_t idx = 0;
					for (auto& instance : groupInstances)
					{
						Float prediction = predictor->Predict(instance);
						//Float prediction = predictions[idx++];

						Float label = instance->label;
						Float weight = instance->weight;

						dvec perInstanceOutputs = gezi::join_vec<double>(datasetMetrics, [&](const DatasetMetricsPtr& datasetMetric)
						{
							return datasetMetric->ProcessInstance(label, prediction, prediction, weight);
						});

						if (isCrossValidationMode)
						{
							gezi::join_vec<double>(globalDatasetMetrics, [&](const DatasetMetricsPtr& datasetMetric)
							{
								return datasetMetric->ProcessInstance(label, prediction, prediction, weight);
							});
						}

						rankingOutputs.emplace_back(perInstanceOutputs);
					}
				}

				dvec perGroupOutpus = gezi::join_vec<double>(datasetMetrics, [&](const DatasetMetricsPtr& datasetMetric)
					{
						return static_pointer_cast<RankingMetrics>(datasetMetric)->FinishGroup();
					});
				
				//����ǽ�����֤Ϊ�˵õ�һ��ȫ�ֽ�� �������ظ�����
				if (isCrossValidationMode)
				{
					gezi::join_vec<double>(globalDatasetMetrics, [&](const DatasetMetricsPtr& datasetMetric)
					{
						return static_pointer_cast<RankingMetrics>(datasetMetric)->FinishGroup();
					});
				}

				if (ofs.is_open())
				{
					size_t innerIdx = 0;
					size_t perInstanceOutputsLength = rankingOutputs[0].size();
					size_t resultLength = perInstanceOutputsLength + perGroupOutpus.size();
					dvec results(resultLength);
					for (size_t i = perInstanceOutputsLength; i < resultLength; i++)
					{
						results[i] = perGroupOutpus[i - perInstanceOutputsLength];
					}
					for (auto& instance : groupInstances)
					{
						for (size_t i = 0; i < perInstanceOutputsLength; i++)
						{
							results[i] = rankingOutputs[innerIdx][i];
						}
						PrintInstanceOutput(idx++, instance, results, groupKey);
						innerIdx++;
					}
				}
			}

			FinalizeProcessInstances();
		}

	protected:

	private:
	};

}  //----end of namespace gezi

#endif  //----end of TESTERS__RANKER_TESTER_H_
