/**
 *  ==============================================================================
 *
 *          \file   Trainers/FastRank/RegressionTree.h
 *
 *        \author   chenghuige
 *
 *          \date   2014-04-10 12:07:25.902949
 *
 *  \Description:
 *  ==============================================================================
 */

#ifndef TRAINERS_FASTRANK__REGRESSION_TREE_H_
#define TRAINERS_FASTRANK__REGRESSION_TREE_H_
#include "common_util.h"
namespace gezi {
	//ֻ�Ƕ��� ͨ��TLC�ı��ļ�
	class RegressionTree
	{
	public:
		~RegressionTree() = default;
		RegressionTree(RegressionTree&&) = default;
		RegressionTree& operator = (RegressionTree&&) = default;
		RegressionTree(const RegressionTree&) = default;
		RegressionTree& operator = (const RegressionTree&) = default;

		RegressionTree(svec& featureNames)
			:_featureNames(featureNames)
		{

		}

	public:

		//�������� ������ ���Ҷ�ӽڵ����ֵ
		Float GetOutput(const Vector& features) const
		{
			int node = 0;
			while (node >= 0)
			{
				PVAL4(node, _featureNames[_splitFeature[node]], features[_splitFeature[node]], _threshold[node]);
				if (features[_splitFeature[node]] <= _threshold[node])
				{
					node = _lteChild[node];
				}
				else
				{
					node = _gtChild[node];
				}
			}
			PVAL2(~node, _leafValue[~node]);
			return _leafValue[~node];
		}

	protected:
	private:
	public:
		dvec _gainPValue;
		ivec _gtChild;
		dvec _leafValue;
		ivec _lteChild;
		double _maxOutput = 0;
		int _numLeaves = 1;
		dvec _previousLeafValue;
		ivec _splitFeature;
		dvec _splitGain;
		dvec _threshold; 
		double _weight = 1.0;
		svec& _featureNames;
	};

}  //----end of namespace gezi

#endif  //----end of TRAINERS_FASTRANK__REGRESSION_TREE_H_
