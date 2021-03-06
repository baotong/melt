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
#include <iostream>
#include "Identifer.h"
#include "file_util.h"
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
      bool ret = false;
      if (endswith(modelPath, ".txt"))
      {
        ret = LoadText(modelPath);
      }
      else
      {
        ret = Load(modelPath);
      }
      if (!ret)
      {
        LOG(FATAL) << "Gbdt load predictor fail";
      }
    }

    /*! \brief if you want to use TLC generated txt model file set to 'TLC' */
    static string& TextFilePattern()
    {
      static string textFilePattern = "LightGBM";
      return textFilePattern;
    }

    //基本不再维护，使用lightGBM训练,读取lightGBM的模型文本即可
    bool LoadTLCText(string file)
    {
      Identifer identifer;

      //_textModelPath = file;
      //LoadSave::Load(file);

      svec lines = read_lines(file);
      if (lines.empty())
        return false;

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
      int numIgnores = GetPredictionKind() == PredictionKind::BinaryClassification ? 2 : 1;
      int treeNum = parse_int_param("Evaluators=", lines[i++]) - numIgnores; //-sum tree -sigmoid
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
        {
          OnlineRegressionTree tree;
          int numInternalNodes = parse_int_param("NumInternalNodes=", lines[i++]);
          tree.NumLeaves = numInternalNodes + 1;

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

          _trees.emplace_back(move(tree));
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

      //--------------设置特征名称
      _featureNames = move(identifer.keys());

      for (auto& tree : _trees)
      {
        tree.SetFeatureNames(_featureNames);
      }

      if (GetPredictionKind() == PredictionKind::BinaryClassification)
      {
        //-------------calibrator
        double paramB = -parse_double_param("Bias=", lines[i]);
        double paramA = -parse_double_param("Weights=", lines[i + 3]);
        Pval2(paramA, paramB);
        _calibrator = make_shared<SigmoidCalibrator>(paramA, paramB);
      }

      return true;
    }

    bool LoadLightGBMText(string file)
    {
      svec lines = read_lines(file);
      if (lines.empty())
        return false;

      size_t i = 0;
      int numFeatures = parse_int_param("max_feature_idx=", lines[i++]) + 1;
      Pval(numFeatures);
      int sigmoid = parse_int_param("sigmoid=", lines[i++]);
      if (sigmoid > 0 && GetPredictionKind() == PredictionKind::BinaryClassification)
      {
        double paramB = 0;
        double paramA = -sigmoid;
        Pval2(paramA, paramB);
        _calibrator = make_shared<SigmoidCalibrator>(paramA, paramB);
      }

      size_t numLines = lines.size();
      while (i < numLines)
      {
        while (i < numLines && !startswith(lines[i], "Tree="))
        {
          i++;
        }
        //pass Tree=
        i++;

        if (i >= numLines)
        {
          break;
        }
        
        OnlineRegressionTree tree;
        
        int numLeaves = parse_int_param("num_leaves=", lines[i++]);
        tree.NumLeaves = numLeaves;

        string splits = parse_string_param("split_feature=", lines[i++]);
        tree._splitFeature = from(split(splits)) >> select([&](string a)
        {
          int index = INT(a);
          CHECK_GE(index, 0);
          return index;
        }) >> to_vector();

        string splitGains = parse_string_param("split_gain=", lines[i++]);
        tree._splitGain = from(split(splitGains)) >> select([](string a) { return DOUBLE(a); }) >> to_vector();

        string thrsholds = parse_string_param("threshold=", lines[i++]);
        tree._threshold = from(split(thrsholds)) >> select([](string a) { return DOUBLE(a); }) >> to_vector();

        string lefts = parse_string_param("left_child=", lines[i++]);
        tree._lteChild = from(split(lefts)) >> select([](string a) { return INT(a); }) >> to_vector();
        string rights = parse_string_param("right_child=", lines[i++]);
        tree._gtChild = from(split(rights)) >> select([](string a) { return INT(a); }) >> to_vector();

        //pass leaf_parent=
        //string leafParent = parse_string_param("leaf_parent=", lines[i++]);
        //tree._parent = from(split(leafParent)) >> select([](string a) { return INT(a); }) >> to_vector();
        i++;

        string outputs = parse_string_param("leaf_value=", lines[i++]);
        tree._leafValue = from(split(outputs)) >> select([](string a) { return DOUBLE(a); }) >> to_vector();

        
        tree.SetNodeValues();

        _trees.emplace_back(move(tree));
      }

      Pval(_trees.size());
      
#ifdef _DEBUG
      //in debug mode always need feature names info
      TryLoadFeatureNamesFromDefaultTextFile();
#endif // _DEBUG
      _featureNames.SetNumFeatures(numFeatures);
      for (auto& tree : _trees)
      {
        tree.SetFeatureNames(_featureNames);
      }
      if (VLOG_IS_ON(1))
      {
        SetFeatureGainVec(ToGainVec(numFeatures));
        std::cout << ToFeaturesGainSummary();
      }
      return true;
    }

    virtual bool LoadText_(string file) override
    {
      SetLoadNormalizerAndCalibrator(false);
      if (TextFilePattern() == "TLC")
      {
        LOG(INFO) << "Loading from text file " << file << " of TLC format";
        return LoadTLCText(file);
      }
      else if (TextFilePattern() == "LightGBM")
      {
        LOG(INFO) << "Loading from text file " << file << " of LightGBM format";
        return LoadLightGBMText(file);
      }
      else
      {
        LOG(WARNING) << "Not supported model text file pattern " << TextFilePattern() << " right now only support LighgtGBM and TLC";
        return false;
      }
    }

    virtual void Save_(string file) override
    {
      //if (!_textModelPath.empty())
      //{ //Hack 拷贝模型文本文件 便于跟踪
      //	string modelTextFile = file + ".txt";
      //	copy_file(_textModelPath, modelTextFile);
      //}
      serialize_util::save(*this, file);
    }

    virtual void CustomSave_(string path) override
    {
      ofstream ofs(path + "/model.bin", ios::binary);
      int ntrees = _trees.size();
      write_elem(ntrees, ofs);
      for (int i = 0; i < ntrees; i++)
      {
        SaveTree(_trees[i], ofs);
      }

      string featureNamesFile = path + "/featureNames.txt";
      _featureNames.Save(featureNamesFile);

      //dynamic will do rtti if not SigmoidCalibrator will be null ptr
      auto calibrator = dynamic_pointer_cast<SigmoidCalibrator>(_calibrator);
      double paramA = -2.0, paramB = 0.0;
      if (calibrator == nullptr)
      {
        VLOG(0) << "No calibrator or other type calibrator will use default param sigmoid calibrator";
      }
      else
      {
        paramA = calibrator->ParamA();
        paramB = calibrator->ParamB();
      }
      write_elem(paramA, ofs);
      write_elem(paramB, ofs);
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

    virtual bool Load_(string file) override
    {
      bool ret = serialize_util::load(*this, file);
      if (!ret)
        return false;
      for (auto& tree : _trees)
      {
        tree.SetFeatureNames(_featureNames);
      }
      return true;
    }

    virtual bool CustomLoad_(string path) override
    {
      std::ifstream ifs(path + "/model.bin", std::ios::binary);
      if (!ifs.is_open())
      {
        LOG(WARNING) << "not find " << path << "/model.bin";
        return false;
      }
      int ntrees;
      read_elem(ifs, ntrees);
      _trees.resize(ntrees);
      for (int i = 0; i < ntrees; i++)
      {
        LoadTree(ifs, _trees[i]);
      }
      double paramA, paramB;
      read_elem(ifs, paramA);
      read_elem(ifs, paramB);
      _calibrator = make_shared<SigmoidCalibrator>(paramA, paramB);

      _featureNames.Load(path + "/featureNames.txt");
      for (auto& tree : _trees)
      {
        tree.SetFeatureNames(_featureNames);
      }

      return true;
    }

    const size_t size() const
    {
      return _trees.size();
    }

    const size_t NumTrees() const
    {
      return _trees.size();
    }

    //TODO 重复代码GainMap and ToGainVec，gbdt训练代码的Ensemble中也有，这里主要用在读取lightGBM数据之后的处理展示，后面Ensemble复用这里代码?
    //统计前prefix棵数目一般用所有的树
    map<int, Float> GainMap(int prefix, bool normalize)
    {
      map<int, Float> m;
      if (_trees.empty())
      {
        return m;
      }
      if ((prefix > NumTrees()) || (prefix < 0))
      {
        prefix = NumTrees();
      }
      for (int i = 0; i < prefix; i++)
      {
        _trees[i].GainMap(m);
      }

      if (normalize)
      {
        for (auto& item : m)
        {
          item.second /= (Float)NumTrees();
        }
      }
      return m;
    }

    vector<Float> ToGainVec(size_t numFeatures, int prefix = -1, bool normalize = true)
    {
      map<int, Float> m = GainMap(prefix, normalize);
      vector<Float> gains(numFeatures, 0);
      for (const auto& item : m)
      {
        gains[item.first] += item.second;
      }
      if (normalize)
      {
        gezi::normalize_vec(gains);
      }
      return gains;
    }

    void PrintFeatureGain(Vector& features, bool sortByAbsGain = true, int level = 0)
    {
      VLOG(level) << "Per feature gain for this predict:\n" <<
        ToGainSummary(features, sortByAbsGain);
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

    virtual string ToGainSummary(Vector& features, bool sortByAbsGain = true) override
    {
      TryLoadFeatureNamesFromDefaultTextFile();
      map<int, double> m = GainMap(features);
      stringstream ss;
      if (sortByAbsGain)
      {
        vector<pair<int, double> > sortedByGain;
        sort_map_by_absvalue_reverse(m, sortedByGain);

        int id = 0;
        for (auto& item : sortedByGain)
        {
          ss << setiosflags(ios::left) << setfill(' ') << setw(40)
            << STR(id++) + ":" + _featureNames[item.first]
            << "\t" << m[item.first]
            << "\t" << item.first << ":" << features[item.first]
            << endl;
        }
      }
      else
      {
        int id = 0;
        for (auto& item : m)
        {
          ss << setiosflags(ios::left) << setfill(' ') << setw(40)
            << STR(id++) + ":" + _featureNames[item.first]
            << "\t" << item.second
            << "\t" << item.first << ":" << features[item.first]
            << endl;
        }
      }

      return ss.str();
    }

#ifdef _DEBUG

    //如果 sortByScore = false, 如果topN < 0 顺序打印所有树的信息,反之 顺序打印topN的树 score
    //如果 sortByScore = true score
    void PrintTreeScores(bool sortByScore = false, bool reverse = true, int maxTreeNum = -1) const
    {
      if (sortByScore)
      {
        if (!reverse)
          sort(_debugNodes.begin(), _debugNodes.end());
        else
          sort(_debugNodes.begin(), _debugNodes.end(), [](const OnlineRegressionTree::DebugNode& l,
          const OnlineRegressionTree::DebugNode& r)
        {
          return l.score < r.score;
        });
      }

      int num = 0;
      for (const OnlineRegressionTree::DebugNode& node : _debugNodes)
      {
        if (!sortByScore || !reverse && node.score >= 0 || reverse && node.score <= 0)
        {
          VLOG(0) << "tree: " << node.id
            << setiosflags(ios::left) << setfill(' ') << setw(7)
            << "\t" << "score: " << node.score
            //<< setiosflags(ios::left) << setfill(' ') << setw(40)
            << "\t" << "depth: " << node.paths.size();
          PVEC(node.paths);
        }
        else
        {
          break;
        }
        num++;
        if (maxTreeNum > 0 && num >= maxTreeNum)
        {
          break;
        }
      }
      VLOG(0) << "Total " << num << " trees show";
    }
#endif // _DEBUG

  protected:
    //注意都是非稀疏的输入应该 稀疏会影响速度 但是不影响结果 而且对于线上 即使稀疏快一点 也无所谓了...
    virtual Float Margin(Vector& features) override
    {
      Float result = 0;
#ifndef _DEBUG
#pragma omp parallel for reduction(+: result)
#endif
      for (size_t i = 0; i < _trees.size(); i++)
      {
        result += _trees[i].Output(features);
#ifdef _DEBUG
        //#pragma  omp critical
        {
          //if (_trees[i]._debugNode.score > 0)
          {
            _trees[i]._debugNode.id = i;
            _debugNodes.emplace_back(move(_trees[i]._debugNode));
          }
        }
#endif // _DEBUG
      }
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

    const vector<OnlineRegressionTree>& Trees() const
    {
      return _trees;
    }

  private:


    void LoadTree(std::ifstream& ifs, OnlineRegressionTree& tree)
    {
      read_elem(ifs, tree.NumLeaves);
      read_elem(ifs, tree._maxOutput);
      read_elem(ifs, tree._weight);
      read_vec(ifs, tree._gainPValue);
      read_vec(ifs, tree._lteChild);
      read_vec(ifs, tree._gtChild);
      read_vec(ifs, tree._leafValue);
      read_vec(ifs, tree._threshold);
      read_vec(ifs, tree._splitFeature);
      read_vec(ifs, tree._splitGain);
      read_vec(ifs, tree._previousLeafValue);
    }

    void SaveTree(const OnlineRegressionTree& tree, ofstream& ofs)
    {
      write_elem(tree.NumLeaves, ofs);
      write_elem(tree._maxOutput, ofs);
      write_elem(tree._weight, ofs);
      write_vec(tree._gainPValue, ofs);
      write_vec(tree._lteChild, ofs);
      write_vec(tree._gtChild, ofs);
      write_vec(tree._leafValue, ofs);
      write_vec(tree._threshold, ofs);
      write_vec(tree._splitFeature, ofs);
      write_vec(tree._splitGain, ofs);
      write_vec(tree._previousLeafValue, ofs);
    }

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
        SaveTreeAsPhpCode(tree, ofs); //复用c code
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
  public: //如果是protected pygcxml不会暴露到python接口中
    vector<OnlineRegressionTree::DebugNode> _debugNodes;
#endif // _DEBUG

#ifdef PYTHON_WRAPPER
  public:
#endif // PYTHON_WRAPPER
    vector<OnlineRegressionTree> _trees;

    //temply used shared between load save function
    //string _textModelPath;
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

  class GbdtRankingPredictor : public GbdtPredictor
  {
    using GbdtPredictor::GbdtPredictor;

    virtual string Name() override
    {
      return "gbdtRanking";
    }

    virtual PredictionKind GetPredictionKind()
    {
      return PredictionKind::Ranking;
    }
  };

}  //----end of namespace gezi
CEREAL_REGISTER_TYPE(gezi::GbdtPredictor);
#endif  //----end of PREDICTORS__GBDT_PREDICTOR_H_
