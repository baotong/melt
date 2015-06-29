/**
*  ==============================================================================
*
*          \file   Predictors/GbdtPredictor.h
*
*        \author   chenghuige
*
*          \date   2014-04-13 18:24:51.525674
*
*  \Description:
*
*  ==============================================================================
*/

#ifndef PREDICTORS__GBDT_PREDICTOR_H_
#define PREDICTORS__GBDT_PREDICTOR_H_
#include "Identifer.h"
#include "MLCore/Predictor.h"
#include "Trainers/Gbdt/OnlineRegressionTree.h"
namespace gezi {

	class GbdtPredictor : public Predictor
	{
	public:
		virtual string Name()
		{
			return "gbdt";
		}

		GbdtPredictor() = default;

		GbdtPredictor(vector<OnlineRegressionTree>& trees, CalibratorPtr calibrator,
			svec&& featureNames)
			:Predictor(calibrator, featureNames)
		{
			_trees.swap(trees);
		}

		GbdtPredictor(vector<OnlineRegressionTree>& trees, svec&& featureNames)
			:Predictor(featureNames)
		{
			_trees.swap(trees);
		}

		GbdtPredictor(vector<OnlineRegressionTree>& trees, CalibratorPtr calibrator,
			const FeatureNamesVector& featureNames)
			:Predictor(calibrator, featureNames)
		{
			_trees.swap(trees);
		}

		GbdtPredictor(vector<OnlineRegressionTree>& trees, const FeatureNamesVector& featureNames)
			:Predictor(featureNames)
		{
			_trees.swap(trees);
		}

		GbdtPredictor(vector<OnlineRegressionTree>& trees)
		{
			_trees.swap(trees);
		}

		GbdtPredictor(string modelPath)
		{
			Load(modelPath);
		}

		//Load Tlc format���ı�ģ���ļ�
		virtual void LoadText(string file) override
		{
			Identifer identifer;

			_textModelPath = file;
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
				identifer.add(lines[i++]);
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
				PVAL(maxLeaves);
				{
					OnlineRegressionTree tree;
					string splits = parse_string_param("SplitFeatures=", lines[i++]);
					tree._splitFeature = from(split(splits, '\t')) >> select([&](string a)
					{
						int index = identifer.id(fnames[INT(split(a, ':')[1]) - 1]);
						CHECK_GE(index, 0);
						return index;
					}) >> to_vector();

					string splitGains = parse_string_param("SplitGain=", lines[i++]);
					tree._splitGain = from(split(splitGains)) >> select([](string a) { return DOUBLE(a); }) >> to_vector();

					string gainPvalues = parse_string_param("GainPValue=", lines[i++]);
					tree._gainPValue = from(split(gainPvalues)) >> select([](string a) { return DOUBLE(a); }) >> to_vector();

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

			//--------------������������
			_featureNames = move(identifer.keys());

			for (auto& tree : _trees)
			{
				tree.SetFeatureNames(_featureNames);
			}

			//-------------calibrator
			double paramB = -parse_double_param("Bias=", lines[i]);
			double paramA = -parse_double_param("Weights=", lines[i + 3]);
			Pval2(paramA, paramB);
			_calibrator = make_shared<SigmoidCalibrator>(paramA, paramB);
		}

		virtual void Save_(string file) override
		{
			if (!_textModelPath.empty())
			{ //Hack ����ģ���ı��ļ� ���ڸ���
				string modelTextFile = file + ".txt";
				copy_file(_textModelPath, modelTextFile);
			}
			serialize_util::save(*this, file);
		}

		virtual void SaveXml_(string file) override
		{
			serialize_util::save_xml(*this, file);
		}

		virtual void SaveJson_(string file) override
		{
			serialize_util::save_json(*this, file);
		}


		virtual void SaveCode_(string file, CodeType codeType) override
		{
			ofstream ofs(file);
			switch (codeType)
			{
			case CodeType::C:
				SaveCCode(ofs);
				break;
			case  CodeType::Php:
				SavePhpCode(ofs);
				break;
			case  CodeType::Python:
				SavePythonCode(ofs);
				break;
			default:
				break;
			}
		}

		virtual void Load_(string file) override
		{
			serialize_util::load(*this, file);
			for (auto& tree : _trees)
			{
				tree.SetFeatureNames(_featureNames);
			}
		}

		void FeatureGainPrint(Vector& features, int level = 0)
		{
			VLOG(level) << "Per feature gain for this predict:\n" <<
				ToGainSummary(features);
		}

		map<int, double> GainMap(Vector& features)
		{
			map<int, double> m;
			if (_trees.empty())
			{
				return m;
			}

			for (auto& tree : _trees)
			{
				tree.GainMap(features, m);
			}

			return m;
		}

		virtual string ToGainSummary(Vector& features) override
		{
			map<int, double> m = GainMap(features);

			vector<pair<int, double> > sortedByGain;
			sort_map_by_absvalue_reverse(m, sortedByGain);

			stringstream ss;
			int id = 0;
			for (auto item : sortedByGain)
			{
				ss << setiosflags(ios::left) << setfill(' ') << setw(40)
					<< STR(id++) + ":" + _featureNames[item.first]
					<< "\t" << m[item.first]
					<< "\t" << item.first << ":" << features[item.first]
					<< endl;
			}
			return ss.str();
		}
	protected:
		//ע�ⶼ�Ƿ�ϡ�������Ӧ�� ϡ���Ӱ���ٶ� ���ǲ�Ӱ���� ���Ҷ������� ��ʹϡ���һ�� Ҳ����ν��...
		virtual Float Margin(Vector& features) override
		{
			Float result = 0;
#pragma omp parallel for reduction(+: result)
			for (size_t i = 0; i < _trees.size(); i++)
			{
				result += _trees[i].Output(features);
#ifdef _DEBUG
#pragma  omp critical
				{
					//if (_trees[i]._debugNode.score > 0)
					{
						_trees[i]._debugNode.id = i;
						_debugNodes.emplace_back(_trees[i]._debugNode);
					}
				}
#endif // _DEBUG
			}
#ifdef _DEBUG
			if (!_reverse)
				sort(_debugNodes.begin(), _debugNodes.end());
			else
				sort(_debugNodes.begin(), _debugNodes.end(), [](const OnlineRegressionTree::DebugNode& l,
				const OnlineRegressionTree::DebugNode& r)
			{
				return l.score < r.score;
			});
			int num = 0;
			for (OnlineRegressionTree::DebugNode& node : _debugNodes)
			{
				if (!_reverse && node.score >= 0 || _reverse && node.score <= 0)
				{
					VLOG(3) << "tree: " << node.id << "\t" << "score: " << node.score << "\t" << "depth: " << node.paths.size();
					PVEC(node.paths);
				}
				else
				{
					VLOG(1) << "Total " << num << " trees show";
					break;
				}
				num++;
			}
#endif // _DEBUG
			return result;
		}

		friend class cereal::access;
		template<class Archive>
		void serialize(Archive &ar, const unsigned int version)
		{
			/*	ar & boost::serialization::base_object<Predictor>(*this);
			ar & _trees;*/
			ar & CEREAL_BASE_OBJECT_NVP(Predictor);
			ar & CEREAL_NVP(_trees);
		}

		vector<OnlineRegressionTree>& Trees()
		{
			return _trees;
		}
#ifdef _DEBUG
		void SetReverse(bool reverse = true)
		{
			_reverse = reverse;
		}
#endif // _DEBUG

	private:
		void SaveCCode(ofstream& ofs)
		{
			int i = 0;
			for (const auto& tree : _trees)
			{
				ofs << "double treeOutput" << i << "=";
				SaveTreeAsCCode(tree, ofs);
				ofs << ";\n";
				i++;
			}
			ofs << "double output = treeOutput0";
			for (int j = 1; j < i; j++)
			{
				ofs << "+treeOuput" << j;
			}
			ofs << ";\n";
		}

		void SavePhpCode(ofstream& ofs)
		{
			int i = 0;
			for (const auto& tree : _trees)
			{
				ofs << "$treeOutput" << i << "=";
				SaveTreeAsPhpCode(tree, ofs); //����c code
				ofs << ";\n";
				i++;
			}
			ofs << "$output = $treeOutput0";
			for (int j = 1; j < i; j++)
			{
				ofs << "+$treeOuput" << j;
			}
			ofs << ";\n";
		}

		void SavePythonCode(ofstream& ofs)
		{
			int i = 0;
			for (const auto& tree : _trees)
			{
				ofs << "treeOutput" << i << "=";
				SaveTreeAsPythonCode(tree, ofs);
				ofs << "\n";
				i++;
			}
			ofs << "output = treeOutput0";
			for (int j = 1; j < i; j++)
			{
				ofs << "+treeOuput" << j;
			}
			ofs << "\n";
		}

		void SaveTreeAsCCode(const OnlineRegressionTree& tree, ofstream& ofs)
		{
			ToCCode(tree, ofs, 0);
		}

		void SaveTreeAsPhpCode(const OnlineRegressionTree& tree, ofstream& ofs)
		{
			ToPhpCode(tree, ofs, 0);
		}

		void SaveTreeAsPythonCode(const OnlineRegressionTree& tree, ofstream& ofs)
		{
			ToPythonCode(tree, ofs, 0);
		}

		// converts a subtree into a freeform expression
		void ToCCode(const OnlineRegressionTree& tree, ofstream& ofs, int node)
		{
			if (node < 0)
			{
				ofs << tree._leafValue[~node];
			}
			else
			{
				ofs << "((" << _featureNames[tree._splitFeature[node]] << " > " << tree._threshold[node] << ") ? ";
				ToCCode(tree, ofs, tree._gtChild[node]);
				ofs << " : ";
				ToCCode(tree, ofs, tree._lteChild[node]);
				ofs << ")";
			}
		}

		void ToPhpCode(const OnlineRegressionTree& tree, ofstream& ofs, int node)
		{
			if (node < 0)
			{
				ofs << tree._leafValue[~node];
			}
			else
			{
				ofs << "(($" << _featureNames[tree._splitFeature[node]] << " > " << tree._threshold[node] << ") ? ";
				ToPhpCode(tree, ofs, tree._gtChild[node]);
				ofs << " : ";
				ToPhpCode(tree, ofs, tree._lteChild[node]);
				ofs << ")";
			}
		}

		void ToPythonCode(const OnlineRegressionTree& tree, ofstream& ofs, int node)
		{
			if (node < 0)
			{
				ofs << tree._leafValue[~node];
			}
			else
			{
				ofs << "(";
				ToPythonCode(tree, ofs, tree._gtChild[node]);
				ofs << " if (" << _featureNames[tree._splitFeature[node]] << " > " << tree._threshold[node] << ") else ";
				ToPythonCode(tree, ofs, tree._lteChild[node]);
				ofs << ")";
			}
		}

	protected:
#ifdef _DEBUG
		vector<OnlineRegressionTree::DebugNode> _debugNodes;
		bool _reverse = false;
#endif // _DEBUG

		vector<OnlineRegressionTree> _trees;

		//temply used shared between load save function
		string _textModelPath;
	};

	class GbdtRegressionPredictor : public GbdtPredictor
	{
	public:
		using GbdtPredictor::GbdtPredictor;

		virtual string Name() override
		{
			return "gbdtRegression";
		}

		virtual PredictionKind GetPredictionKind()
		{
			return PredictionKind::Regression;
		}
	};
}  //----end of namespace gezi
CEREAL_REGISTER_TYPE(gezi::GbdtPredictor);
#endif  //----end of PREDICTORS__GBDT_PREDICTOR_H_