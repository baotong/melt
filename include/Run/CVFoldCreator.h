/**
 *  ==============================================================================
 *
 *          \file   Run/CVFoldCreator.h
 *
 *        \author   chenghuige
 *
 *          \date   2014-03-31 07:18:51.652013
 *
 *  \Description:
 *  ==============================================================================
 */

#ifndef RUN__C_V_FOLD_CREATOR_H_
#define RUN__C_V_FOLD_CREATOR_H_
#include "Prediction/Instances/Instance.h"
#include "Run/MeltArguments.h"
namespace gezi {

	class CVFoldCreator
	{
	public:
		static ivec CreateFoldIndices(const Instances& data, const MeltArguments& cmd, const RandomEngine& rng)
		{
			//没有任何随机性 顺序划分
			if (cmd.foldsSequential)
				return CreateFoldIndicesSequential(data, cmd.numFolds, rng);

			if (data.IsRankingInstances())
			{ //rank需要考虑 相同key在一组,内部 randIdx 所以不需要 整体先随机
				return CreateFoldIndicesStratified(data, cmd.numFolds, rng);
			}

			data.Randomize(rng);

			//默认最常用的二分类 确保正负比例一致
			if (data.IsBinaryClassificationInstances())
			{
				return CreateFoldIndicesBalanced(data, cmd.numFolds);
			}
			else
			{ //regresssion,多分类 @TODO 多分类也保持比例 类似二分类？
				return CreateFoldIndicesOneByOne(data, cmd.numFolds);
			}
		}

		//对每一个instance给一个分组编号，确保每一组的正反比例一致 most commonly used
		//和TLC保持一致
		static ivec CreateFoldIndicesBalanced(const Instances& data, int numFolds)
		{
			ivec foldIndices(data.size(), -1);
			int numPos = 0, numNeg = 0, numUnlabeled = 0;
			for (InstancePtr instance : data)
			{
				if (instance->label > 0)
					numPos++;
				else if (instance->label <= 0)
					numNeg++;
				else
					numUnlabeled++;
			}
			int numPosPerFold = numPos / numFolds;
			int numNegPerFold = numNeg / numFolds;
			int numUnlabeledPerFold = numUnlabeled / numFolds;

			VLOG(0) << boost::format("In every fold, positives: %d negatives: %d unlabeled: %d") % numPosPerFold % numNegPerFold % numUnlabeledPerFold;

			int currPosFold = 0, currNegFold = 0, numCurrPos = 0, numCurrNeg = 0;
			for (size_t i = 0; i < data.size(); i++)
			{
				if (data[i]->label > 0)
				{
					foldIndices[i] = currPosFold;
					numCurrPos++;
					if (numCurrPos >= numPosPerFold)
					{
						currPosFold++;
						if (currPosFold >= numFolds)
						{
							currPosFold = 0;
							numCurrPos = numPosPerFold;
						}
						else
						{
							numCurrPos = 0;
						}
					}
				}
				else
				{
					foldIndices[i] = currNegFold;
					numCurrNeg++;
					if (numCurrNeg >= numNegPerFold)
					{
						currNegFold++;
						if (currNegFold >= numFolds)
						{
							currNegFold = 0;
							numCurrNeg = numNegPerFold;
						}
						else
						{
							numCurrNeg = 0;
						}
					}
				}
			}
			return foldIndices;
		}

		//@TODO 如果先做随机化 然后 0,1,2,3,0,1,2,3 这样均匀分就好了。。,或者直接 每个位置随机一个index也ok
		/// assign each instance to a fold sequentially.
		/// To "even out" the remaninder, randomly assign an extra instance to every fold
		static ivec CreateFoldIndicesSequential(const Instances& data, int numFolds, const RandomEngine& rng)
		{
			VLOG(1) << "Create " << numFolds << " folds by sequential assignment";

			ivec foldIndices(data.size(), -1);
			ivec foldCnts(numFolds, 0);

			RandomDouble rand(rng);

			int instancesPerFold = data.size() / numFolds;
			int instanceIdx = 0;
			int moduloInstancesAssigned = 0;

			for (int foldIdx = 0; foldIdx < numFolds; foldIdx++)
			{
				for (int j = 0; j < instancesPerFold; j++)
				{
					foldIndices[instanceIdx++] = foldIdx;
					foldCnts[foldIdx]++;
				}
				// assign one extra for remainder to get balanced folds
				if (instanceIdx < data.size() && rand.Next() < (1.0 * (data.size() % numFolds) - moduloInstancesAssigned) / (numFolds - foldIdx))
				{
					foldIndices[instanceIdx++] = foldIdx;
					foldCnts[foldIdx]++;
					moduloInstancesAssigned++;
				}
			}
			while (instanceIdx < data.size())
			{
				foldIndices[instanceIdx++] = numFolds - 1;
				foldCnts[numFolds - 1]++;
			}

			PVEC(foldCnts);

			return foldIndices;
		}

		static ivec CreateFoldIndicesOneByOne(const Instances& data, int numFolds)
		{
			ivec foldIndices(data.size(), -1);
			int foldIdx = 0;
			for (size_t i = 0; i < data.size(); i++)
			{
				foldIndices[i] = foldIdx;
				foldIdx = (foldIdx + 1) % numFolds;
			}
			return foldIndices;
		}

		//@TODO 指定特定的 name,attibute 比如 username 那么相同userid的instance 会分到相同group
		//对于比如urate样本集合 可能会避免 类似样本同时作为训练和测试样本，类似的包括比如
		//query,url 对 可能需要对相同query 放到相同group
		//this is *EXTREMELY* important if your data has multiple examples for same “item” – e.g., 
		//multiple keywords for the same query.  It is then critical that all items for the query 
		//are in the same fold (otherwise the learner “cheats” by seeing examples for same query).  
		static ivec CreateFoldIndicesStratified(const Instances& data, int numFolds, const RandomEngine& rng)
		{
			VLOG(0) << "Creating " << numFolds << " folds using stratified sampling by " << gezi::join(data.schema.groupKeys, "|");
			ivec foldIndices(data.size(), -1);
			ivec cnt(numFolds, 0);

			RandomInt rand(numFolds, rng);
			unordered_map<string, int> nameFoldDict;
			int idx = 0;
			for(const auto& instance : data)
			{
				string key = instance->groupKey;
				int foldIdx;
				auto iter = nameFoldDict.find(key);
				if (iter == nameFoldDict.end())
				{
					foldIdx = rand.Next();
					nameFoldDict[key] = foldIdx;
				}
				else
				{
					foldIdx = iter->second;
				}
				foldIndices[idx++] = foldIdx;
				cnt[foldIdx]++;
			}

			Float avgInstancesPerFold = (Float)data.size() / numFolds;
			for (int i = 0; i < numFolds; i++)
			{
				VLOG(0) << "\tFold " << i << " has " << cnt[i] << "  instances";
				if (std::abs(cnt[i] - avgInstancesPerFold) > 0.1 * avgInstancesPerFold)
				{
					LOG(WARNING) << "stratified sampling results in unabalanced folds";
				}
				CHECK_GT(cnt[i], 0);
			}

			return foldIndices;
		}

		static void CreateFolds(const Instances& data, Float trainProportion,
			ivec& instanceFoldIndices, int foldIdx, int numFolds,
			Instances& trainData, Instances& testData,
			const RandomEngine& rng)
		{
			trainData.CopySchema(data.schema);
			testData.CopySchema(data.schema);

			//@TODO 样本数目不会超过int
			// assign instances to either testData (if instance is in foldIdx), or trainData
			for (int i = 0; i < data.Size(); i++)
			{
				if (instanceFoldIndices[i] == foldIdx)
				{
					testData.Add(data[i]);
				}
				else if (instanceFoldIndices[i] < numFolds)
				{
						trainData.Add(data[i]);
				}
			}

			// remove (1-trainProportion)% of instances from the training fold if necessary
			// (for learning curve experiments)
			trainData.ShrinkData(trainProportion, rng);
		}
	};

}  //----end of namespace gezi

#endif  //----end of RUN__C_V_FOLD_CREATOR_H_
