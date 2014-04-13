/**
 *  ==============================================================================
 *
 *          \file   Predictors/FastRankPredictor.h
 *
 *        \author   chenghuige
 *
 *          \date   2014-04-13 18:24:51.525674
 *
 *  \Description:  FastRank����ʱhack�� ��ʵ��ѵ��֮ǰ ��һ����ȡ�������������Ԥ���
 *                 Predictor �����õ��Ķ��� �����Ӽ�� ������Ԥ�⼴�� �ŵ�FastRankĿ¼
 *                 ����ʵ���㷨�� ��Ȼ������� ��ʵ�ֵķ���BoostedTree Predictor�ͽ�Ϊ
 *                 BoostedTreePredictor ʹ��ʱ�� fastrank boostedtree gbdt����
 *  ==============================================================================
 */

#ifndef PREDICTORS__FAST_RANK_PREDICTOR_H_
#define PREDICTORS__FAST_RANK_PREDICTOR_H_
#include "Identifer.h"
#include "MLCore/Predictor.h"
#include "Trainers/FastRank/RegressionTree.h"
namespace gezi {

	//hack ��ǰδʵ��Fastrank��ѵ�� ��������ʵ��Load ��Load Tlc format���ı�ģ���ļ�
	class FastRankPredictor : public Predictor
	{
	public:
		virtual string Name() override
		{
			return "FastRankPredictor";
		}

		virtual void Load(string file) override
		{
			LoadSave::Load(file);

			svec lines = read_lines(file);

			size_t i = 0;
			for (; i < lines.size(); i++)
			{
				if (startswith(lines[i], "[FeatureNames]"))
				{
					i++;
					break;
				}
			}
			int featureNum = parse_int_param("FeatureNum=", lines[i++]);
			Pval(featureNum);
			for (int j = 0; j < featureNum; j++)
			{
				_identifer.add(lines[i++]);
			}

			for (; i < lines.size(); i++)
			{
				if (startswith(lines[i], "[TreeEnsemble]"))
				{
					i++;
					break;
				}
			}

			int inputs = parse_int_param("Inputs=", lines[i++]);
			Pval(inputs);
			int treeNum = parse_int_param("Evaluators=", lines[i++]) - 2; //-sum tree -sigmoid
			Pval(treeNum);

			svec fnames(inputs);
			for (int j = 0; j < inputs; j++)
			{
				for (; i < lines.size(); i++)
				{
					if (startswith(lines[i], "[Input:"))
					{
						i += 2;
						break;
					}
				}
				fnames[j] = parse_string_param("Name=", lines[i++]);
			}

			for (int j = 0; j < treeNum; j++)
			{
				for (; i < lines.size(); i++)
				{
					if (startswith(lines[i], "[Evaluator:"))
					{
						i += 2;
						break;
					}
				}
				int maxLeaves = parse_int_param("NumInternalNodes=", lines[i++]);
				{
					RegressionTree tree(_identifer.keys());
					string splits = parse_string_param("SplitFeatures=", lines[i++]);
					tree._splitFeature = from(split(splits, '\t')) >> select([&fnames, this](string a)
					{
						int index = _identifer.id(fnames[INT(split(a, ':')[1]) - 1]);
						CHECK_GE(index, 0);
						return index;
					}) >> to_vector();

					i += 2;
					string lefts = parse_string_param("LTEChild=", lines[i++]);
					tree._lteChild = from(split(lefts)) >> select([](string a) { return INT(a); }) >> to_vector();
					string rights = parse_string_param("GTChild=", lines[i++]);
					tree._gtChild = from(split(rights)) >> select([](string a) { return INT(a); }) >> to_vector();

					string thrsholds = parse_string_param("Threshold=", lines[i++]);
					tree._threshold = from(split(thrsholds)) >> select([](string a) { return DOUBLE(a); }) >> to_vector();

					string outputs = parse_string_param("Output=", lines[i++]);
					tree._leafValue = from(split(outputs)) >> select([](string a) { return DOUBLE(a); }) >> to_vector();
					_trees.emplace_back(tree);
				}
			}

			for (; i < lines.size(); i++)
			{
				if (startswith(lines[i], "[Evaluator:"))
				{
					i += 2;
					break;
				}
			}

			for (; i < lines.size(); i++)
			{
				if (startswith(lines[i], "[Evaluator:"))
				{
					i += 3;
					break;
				}
			}
			double paramB = -parse_double_param("Bias=", lines[i]);
			double paramA = -parse_double_param("Weights=", lines[i + 3]);
			PVAL2(paramA, paramB);
			_calibrator = make_shared<SigmoidCalibrator>(paramA, paramB);
		}
	protected:
		//ע�ⶼ�Ƿ�ϡ�������Ӧ�� ϡ���Ӱ���ٶ� ���ǲ�Ӱ���� ���Ҷ������� ��ʹϡ���һ�� Ҳ����ν��...
		virtual Float Margin(Vector& features) override
		{
			Float result = 0;
#pragma omp parallel for reduction(+: result)
			for (size_t i = 0; i < _trees.size(); i++)
			{
				result += _trees[i].GetOutput(features);
#ifdef _DEBUG
#pragma  omp critical
				{
					if (_trees[i]._debugNode.score > 0)
					{
						_trees[i]._debugNode.id = i;
						_debugNodes.emplace_back(_trees[i]._debugNode);
					}
				}
#endif // _DEBUG
			}

#ifdef _DEBUG
			sort(_debugNodes.begin(), _debugNodes.end());
			for (RegressionTree::DebugNode& node : _debugNodes)
			{
				VLOG(3) << node.id << "\t" << node.score << "\t" << node.paths.size();
				PVEC(node.paths);
			}
#endif // _DEBUG
			return result;
		}
	private:
#ifdef _DEBUG
		vector<RegressionTree::DebugNode> _debugNodes;
#endif // _DEBUG

		vector<RegressionTree> _trees;
		Identifer _identifer;
	};

}  //----end of namespace gezi

#endif  //----end of PREDICTORS__FAST_RANK_PREDICTOR_H_
