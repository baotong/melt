/**
 *  ==============================================================================
 *
 *          \file   Trainers/FastRank/OnlineRegressionTree.h
 *
 *        \author   chenghuige
 *
 *          \date   2014-04-10 12:07:25.902949
 *
 *  \Description:
 *  ==============================================================================
 */

#ifndef TRAINERS_FASTRANK__ONLINE_REGRESSION_TREE_H_
#define TRAINERS_FASTRANK__ONLINE_REGRESSION_TREE_H_
#include "common_util.h"
namespace gezi {
	//只是读入 通过TLC文本文件
	class FastRankPredictor;
	class OnlineRegressionTree
	{
	public:
		friend class FastRankPredictor;
		~OnlineRegressionTree() = default;
		OnlineRegressionTree() = default;
		OnlineRegressionTree(OnlineRegressionTree&&) = default;
		OnlineRegressionTree& operator = (OnlineRegressionTree&&) = default;
		OnlineRegressionTree(const OnlineRegressionTree&) = default;
		OnlineRegressionTree& operator = (const OnlineRegressionTree&) = default;

		OnlineRegressionTree(svec& featureNames)
			:_featureNames(&featureNames)
		{

		}
#ifdef _DEBUG
		struct DebugNode
		{
			int id;
			svec paths;
			double score;
			bool operator < (const DebugNode& other) const
			{
				return score > other.score;
			}
		};
#endif

	public:

		//输入特征 遍历树 输出叶子节点的数值
		Float Output(Vector& features)
		{
			int node = 0;
			while (node >= 0)
			{
				if (features[_splitFeature[node]] <= _threshold[node])
				{
#ifdef _DEBUG
					/*_debugNode.paths.push_back((*_featureNames)[_splitFeature[node]] + " " +
						STR(features[_splitFeature[node]]) + " <= " + STR(_threshold[node])
						+ " " + STR(_splitGain[node]) + " " + STR(_gainPValue[node]));*/
					string result = (format("%s %.5f <= %.5f") % (*_featureNames)[_splitFeature[node]] %
						features[_splitFeature[node]] % _threshold[node]).str();
					_debugNode.paths.push_back(result);
#endif // _DEBUG
					node = _lteChild[node];
				}
				else
				{
#ifdef _DEBUG
					/*_debugNode.paths.push_back((*_featureNames)[_splitFeature[node]] + " " +
						STR(features[_splitFeature[node]]) + " > " + STR(_threshold[node])
						+ " " + STR(_splitGain[node]) + " " + STR(_gainPValue[node]));*/
					string result = (format("%s %.5f > %.5f") % (*_featureNames)[_splitFeature[node]] %
						features[_splitFeature[node]] % _threshold[node]).str();
					_debugNode.paths.push_back(result);
#endif // _DEBUG
					node = _gtChild[node];
				}
			}
#ifdef _DEBUG
			{
				_debugNode.score = _leafValue[~node];
			}
#endif // _DEBUG
			return _leafValue[~node];
		}

		void GainMap(Vector& features, map<int, double>& m)
		{
			if (NumLeaves == 1)
			{
				return;
			}
			int node = 0, preNode = 0;
			while (node >= 0)
			{
				preNode = node;
				if (features[_splitFeature[node]] <= _threshold[node])
				{
					node = _lteChild[node];
				}
				else
				{
					node = _gtChild[node];
				}

				double nowValue = node >= 0 ? _previousLeafValue[node] : _leafValue[~node];
				add_value(m, _splitFeature[preNode], nowValue - _previousLeafValue[preNode]);
			}
		}

		void Print(int node = 0, int depth = 0)
		{
			for (int i = 0; i < depth; i++)
			{
				cout << "|  ";
			}
			if (node < 0)
			{
				cout << _leafValue[~node] << endl;
			}
			else
			{
				cout << (*_featureNames)[_splitFeature[node]] << " <= " << _threshold[node] << " ?" << endl;
				Print(_lteChild[node], depth + 1);
				Print(_gtChild[node], depth + 1);
			}
		}

		void Print(Vector& features, int node = 0, int depth = 0, string suffix = "$")
		{
			for (int i = 0; i < depth; i++)
			{
				cout << "|  ";
			}
			cout << suffix;

			if (node < 0)
			{
				if (!suffix.empty())
					cout << "[" << _leafValue[~node] << "]" << endl;
				else
					cout << _leafValue[~node] << endl;
			}
			else
			{
				cout << format("%s %.5f <= %.5f ?") % (*_featureNames)[_splitFeature[node]] %
					features[_splitFeature[node]] % _threshold[node] << endl;
				string lsuffix = "", rsuffix = "";
				if (!suffix.empty())
				{
					if (features[_splitFeature[node]] <= _threshold[node])
						lsuffix = "$";
					else
						rsuffix = "$";
				}
				Print(features, _lteChild[node], depth + 1, lsuffix);
				Print(features, _gtChild[node], depth + 1, rsuffix);
			}
		}

	public:
		friend class cereal::access;
		template<class Archive>
		void serialize(Archive &ar, const unsigned int version)
		{
		/*	ar & NumLeaves;

			ar & _gainPValue;
			ar & _gtChild;
			ar & _leafValue;
			ar & _lteChild;
			ar & _maxOutput;
			ar & _previousLeafValue;
			ar & _splitFeature;
			ar & _splitGain;
			ar & _threshold;
			ar & _weight;*/
			//ar & _featureNames; //不要序列化指针

			ar & GEZI_SERIALIZATION_NVP(NumLeaves);

			ar & GEZI_SERIALIZATION_NVP(_gainPValue); 
			ar & GEZI_SERIALIZATION_NVP(_gtChild);
			ar & GEZI_SERIALIZATION_NVP(_leafValue);
			ar & GEZI_SERIALIZATION_NVP(_lteChild);
			ar & GEZI_SERIALIZATION_NVP(_maxOutput);
			ar & GEZI_SERIALIZATION_NVP(_previousLeafValue);
			ar & GEZI_SERIALIZATION_NVP(_splitFeature);
			ar & GEZI_SERIALIZATION_NVP(_splitGain);
			ar & GEZI_SERIALIZATION_NVP(_threshold);
			ar & GEZI_SERIALIZATION_NVP(_weight);
		}
#ifdef _DEBUG
		DebugNode _debugNode;
#endif // _DEBUG
	public:
		int NumLeaves = 1;
	protected:
		dvec _gainPValue;
		ivec _lteChild;
		ivec _gtChild;
		dvec _leafValue;
		dvec _threshold;
		ivec _splitFeature;
		dvec _splitGain;
		double _maxOutput = 0;
		dvec _previousLeafValue;
		double _weight = 1.0;
		svec* _featureNames;
	};

}  //----end of namespace gezi

#endif  //----end of TRAINERS_FASTRANK__ONLINE_REGRESSION_TREE_H_
