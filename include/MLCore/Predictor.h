/**
 *  ==============================================================================
 *
 *          \file   MLCore/Predictor.h
 *
 *        \author   chenghuige
 *
 *          \date   2014-04-07 22:07:30.510777
 *
 *  \Description:
 *  ==============================================================================
 */

#ifndef M_L_CORE__PREDICTOR_H_
#define M_L_CORE__PREDICTOR_H_

#include "common_util.h"
#include "serialize_util.h"
#include "statistic_util.h"

#include "Prediction/Normalization/NormalizerFactory.h"
#include "Prediction/Calibrate/CalibratorFactory.h"
#include "Prediction/Instances/Instance.h"
#include "Prediction/Normalization/Normalizer.h"
#include "Prediction/Calibrate/Calibrator.h"
#include "PredictionKind.h"

namespace gezi {

  enum class CodeType
  {
    C = 0,
    Python = 1,
    Php = 2
  };

  //当前只是考虑 二分类模型
  //@TODO 多分类模型支持 重载 Fvec 或者Predict返回Fvec ?, Regression Rank等模型的支持
  //@TODO TLC c#的那种接口的设计 能够迁移到c++吗 c#的关键有一个  object Predict(object features);   public interface IPredictor<in TFeatures, out TResult> : IPredictor
  //TResult Predict(TFeatures features);
  //var stronglyTypedPredictor = predictor as IPredictor<Instance, float>;
  //dvec Predicts(const Vector&) 
  //double Predict(const Vector&, dvec&)
  class EnsemblePredictor;
  //--hack for castxml
  class LoadSave;
  class Predictor : public LoadSave
  {
  public:
    Predictor() = default;
    virtual ~Predictor() {}

    //fully create like LinearSVM 
    Predictor(NormalizerPtr normalizer, CalibratorPtr calibrator, const FeatureNamesVector& featureNames)
      :_normalizer(normalizer), _calibrator(calibrator), _featureNames(featureNames)
    {
    }


    //no normalizer like Gbdt BinaryClassification
    Predictor(CalibratorPtr calibrator, const FeatureNamesVector& featureNames)
      :_calibrator(calibrator), _featureNames(featureNames)
    {

    }

    //no calibrator like Gbdt regression
    Predictor(const FeatureNamesVector& featureNames)
      :_featureNames(featureNames)
    {

    }

    virtual PredictionKind GetPredictionKind()
    {
      return PredictionKind::BinaryClassification;
    }

    virtual string Name() override
    {
      return "Predictor";
    }

    Predictor& SetPath(string path)
    {
      _path = path;
      return *this;
    }

    string Path()
    {
      return _path;
    }

    Predictor& SetParam(string param)
    {
      _param = param;
      return *this;
    }

    Predictor& SetUseCustomModel(bool useCustomModel)
    {
      _useCustomModel = useCustomModel;
      return *this;
    }

    bool UseCustomModel() const
    {
      return _useCustomModel;
    }

    string GetParam()
    {
      return _param;
    }

    //输出未经Calibrator矫正的数值 -n,+n 0表示分界 越高越倾向positive
    Float Output(Instance& instance)
    {
      return Output(instance.features);
    }

    //在Melt.h 统一使用虚函数接口  输入是 InstancePtr
    virtual Float Output(InstancePtr instance)
    {
      return Output(instance->features);
    }

    map<string, Float> ToNamedFeatures(const Vector& features)
    {
      map<string, Float> namedFeatures;
      features.ForEachNonZero([this, &namedFeatures](int index, Float value)
      {
        namedFeatures[_featureNames[index]] = value;
      });
      return namedFeatures;
    }

    void Print(const Vector& features, bool nonZeroOnly = false, std::ostream& ofs = std::cout)
    {
      if (nonZeroOnly)
      {
        features.ForEachNonZero([this, &ofs](int index, Float value)
        {
          ofs << index << "\t" << _featureNames[index] << "\t" << value << std::endl;
        });
      }
      else
      {
        features.ForEachAllIf([this, &ofs](int index, Float value)
        {
          ofs << index << "\t" << _featureNames[index] << "\t" << value << std::endl;
          if (index == (int)_featureNames.size() - 1)
          {
            return false;
          }
          return true;
        });
      }
    }

    //@TODO 考虑尽可能 输入 const 否则容易引起混淆错误
    Float Output(Vector& features)
    {
      if (_normalizer != nullptr && !features.normalized)
      {
        if (!_normalizeCopy)
        {
          _normalizer->Normalize(features);
          return Margin(features);
        }
        else
        {
          Vector temp = _normalizer->NormalizeCopy(features);
          return Margin(temp);
        }
      }
      else
      {
        return Margin(features);
      }
    }

    Float Output(Fvec& values)
    {
      Vector features(values);
      return Output(features);
    }

    Float Output(ivec& indices, Fvec& values)
    {
      Vector features(indices, values);
      return Output(features);
    }

    //输出经Calibrator矫正的数值 [0,1], 输出值表示结果倾向positive的概率
    Float Predict(Instance& instance)
    {
      return Predict(instance.features);
    }

    Float Predict(Fvec& values)
    {
      Vector features(values);
      return Predict(features);
    }

    Float Predict(ivec& indices, Fvec& values)
    {
      Vector features(indices, values);
      return Predict(features);
    }

    //-------- wrapper for string input
    virtual Float Output(string featureStr)
    {
      Vector fe(featureStr);
      return Output(fe);
    }

    virtual Float Predict(string featureStr)
    {
      Vector fe(featureStr);
      return Predict(fe);
    }

    virtual Float Predict(string featureStr, Float& output)
    {
      Vector fe(featureStr);
      return Predict(fe, output);
    }
    //-------- wrapper for string input end


    virtual Float Predict(InstancePtr instance)
    {
      return Predict(instance->features);
    }

    //@TODO  提供const 版本
    virtual Float Predict(Vector& features)
    {
      //if (GetPredictionKind() == PredictionKind::Regression
      //	|| GetPredictionKind() == PredictionKind::MultiOutputRegression)
      if (GetPredictionKind() == PredictionKind::BinaryClassification)
      {
        return Predict(Output(features));
      }
      else
      {
        return Output(features);
      }
    }

    //virtual vector<Float> BulkPredict(vector<Vector>& featuresVector)
    vector<Float> BulkPredict(vector<Vector>& featuresVector)
    {
      vector<Float> results(featuresVector.size());
#pragma omp parallel for
      for (size_t i = 0; i < featuresVector.size(); i++)
      {
        results[i] = Predict(featuresVector[i]);
      }
      return results;
    }

    vector<Float> BulkPredict(vector<InstancePtr>& instances)
    {
      vector<Float> results(instances.size());
#pragma omp parallel for
      for (size_t i = 0; i < instances.size(); i++)
      {
        results[i] = Predict(instances[i]);
      }
      return results;
    }

    vector<Float> BulkPredict(vector<InstancePtr>& instances, vector<Float>& outputs)
    {
      vector<Float> results(instances.size());
      outputs.resize(instances.size());
#pragma omp parallel for
      for (size_t i = 0; i < instances.size(); i++)
      {
        results[i] = Predict(instances[i], outputs[i]);
      }
      return results;
    }

    virtual void InitThread()
    {

    }

    Float Predict(Instance& instance, Float& output)
    {
      return Predict(instance.features, output);
    }

    virtual Float Predict(InstancePtr instance, Float& output)
    {
      return Predict(instance->features, output);
    }

    //@TODO maybe use RegressionPredictor is better ? but this is easiest way
    Float Predict(Vector& features, Float& output)
    {
      output = Output(features);
      if (GetPredictionKind() == PredictionKind::BinaryClassification)
      {
        return Predict(output);
      }
      else
      {
        return output;
      }
    }

    Float Predict(Fvec& values, Float& output)
    {
      Vector features(values);
      return Predict(features, output);
    }

    Float Predict(ivec& indices, Fvec& values, Float& output)
    {
      Vector features(indices, values);
      return Predict(features, output);
    }

    Float Predict(const map<int, double>& m)
    {
      Vector features(m);
      return Predict(features);
    }

    Float Output(const map<int, double>& m)
    {
      Vector features(m);
      return Output(features);
    }

    Float Predict(const map<int, double>& m, Float& output)
    {
      Vector features(m);
      return Predict(features, output);
    }

    virtual Float Predict(Float output)
    {
      if (_calibrator != nullptr)
      {
        return _calibrator->PredictProbability(output);
      }
      else
      { //f(x) = (1/2) * log(pr(true) / 1 - pr(true))
        //@TODO 按照fastrank的优化目标函数output->prob推导 应该是 -2*output 但是似乎这样比 -output对应的logloss要大 check 是否不同数据集合不一定
        //return 1.0 / (1.0 + exp(-2.0 * output));
        //return 1.0 / (1.0 + exp(-output)); // == gezi::sigmoid(output)
        //但是注意较大的值可能会都映射到0或者1从而损失排序信息,auc会变低
        return gezi::sigmoid(output);
      }
    }

    Predictor& SetNormalizeCopy(bool normalizeCopy = true)
    {
      _normalizeCopy = normalizeCopy;
      return *this;
    }

    //一般不调用 调用Save(path)
    virtual void Save()
    {
      if (_useCustomModel)
      {
        CustomSave(_path);
      }
      else
      {
        Save(_path);
      }
    }

    virtual void SaveBin(string path)
    {
      string modelFile = path + "/model.bin";
      Save_(modelFile);
    }

    virtual void Save(string path) override
    {
      if (_useCustomModel)
      {
        CustomSave(_path);
      }
      else
      {
        LoadSave::Save(path);
        _path = path;
        try_create_dir(path);
        SaveBin(path);
        write_file(Name(), path + "/model.name.txt");
        write_file(_param, path + "/model.param.txt");
        SAVE_SHARED_PTR(_normalizer, path);
        SAVE_SHARED_PTR(_calibrator, path);
      }
    }

    virtual void CustomSave()
    {
      CustomSave(_path);
    }

    virtual void CustomSave(string path) override
    {
      LoadSave::CustomSave(path);
      _path = path;
      try_create_dir(path);
      CustomSave_(path);
      write_file(Name(), path + "/model.name.txt");
      write_file(_param, path + "/model.param.txt");
    }

    virtual void CustomSave_(string path)
    {
      LOG(WARNING) << Name() << " currently not support custom save";
    }

    virtual void Save_(string file)
    {
      LOG(WARNING) << Name() << " currently not support saving model";
    }

    bool LoadNormalizerAndCalibrator(string path)
    {
      bool ret = true;
      if (loadNormalizerAndCalibrator())
      {
#ifdef NO_CEREAL
        string normalizerName = read_file(OBJ_NAME_PATH(_normalizer, path));
        if (!normalizerName.empty())
        { //理论上有了cereal序列化多态shared ptr机制 不再需要单独的通过路径载入这个函数 走下面的默认路径
          _normalizer = NormalizerFactory::CreateNormalizer(normalizerName, OBJ_PATH(_normalizer, path));
          if (!_normalizer)
            return false;
        }
        string calibratorName = read_file(OBJ_NAME_PATH(_calibrator, path));
        if (!calibratorName.empty())
        {
          _calibrator = CalibratorFactory::CreateCalibrator(calibratorName, OBJ_PATH(_calibrator, path));
          if (!_calibrator)
            return false;
        }
#else 
        //现在走这个路径
        ret = serialize_util::load(_normalizer, OBJ_PATH(_normalizer, path));
        if (!ret)
          return ret;
        ret = serialize_util::load(_calibrator, OBJ_PATH(_calibrator, path));
        if (!ret)
          return ret;
#endif
      }
      return ret;
    }

    virtual bool LoadBin(string path)
    {
      string modelFile = endswith(path, ".bin") ? path : path + "/model.bin";
      return Load_(modelFile);
    }

    void ReadPathAndSetParam_(string path)
    {
      _path = path;
      _param = read_file(OBJ_NAME_PATH(_param, path));
    }

    virtual bool CustomLoad(string path) override
    {
      ReadPathAndSetParam_(path);
      LoadSave::CustomLoad(path);
      return CustomLoad_(path);
    }

    virtual bool Load(string path) override
    {
      if (_useCustomModel)
      {
        return CustomLoad(path);
      }
      ReadPathAndSetParam_(path);
      LoadSave::Load(path);
      bool ret = true;
      ret = LoadBin(path);
      if (!ret)
        return ret;
      ret = LoadNormalizerAndCalibrator(path);
      return ret;
    }

    virtual bool Load_(string file)
    {
      LOG(FATAL) << Name() << " currently not support loading model";
      return true;
    }

    //注意不管是save xml还是save text 都不要单独使用 并且 都在Save 也就是存储二进制之后使用
    //xml和text都提供载入 目前 都是按照二进制载入 如果是其它的开源输出比如linear model格式可以先转LinearPredictor然后
    //Save 
    virtual void SaveXml(string file) override
    {
      LoadSave::SaveXml(file);
      SaveXml_(file);
      SaveXml_NormalizerAndCalibrator(_saveNormalizerText, _saveCalibratorText);
    }

    virtual void SaveXml_(string file)
    {
      LOG(WARNING) << Name() << " currently not support saving as xml";
    }

    virtual void SaveJson(string file) override
    {
      LoadSave::SaveJson(file);
      SaveJson_(file);
      SaveJson_NormalizerAndCalibrator(_saveNormalizerText, _saveCalibratorText);
    }

    //only need to save model for code not care about normalizer or calibrator
    //但是类似的也是需要先Save(path) 设定路径 也就是说必须在存储bin model之后,存储bin是必须,存储code是optional
    virtual void SaveCode(string codeTypeStr = "cpp")
    {
      string file = _path + "/model." + codeTypeStr;
      CodeType codeType = CodeType::C;
      if (codeTypeStr == "py" || codeTypeStr == "python")
      {
        codeType = CodeType::Python;
        VLOG(0) << "Save " << file << " as python code";
      }
      else if (codeTypeStr == "php")
      {
        codeType = CodeType::Php;
        VLOG(0) << "Save " << file << " as php code";
      }
      else
      {
        VLOG(0) << "Save " << file << " as c/c++ code";
      }
      SaveCode_(file, codeType);
    }

    virtual void SaveJson_(string file)
    {
      LOG(WARNING) << Name() << " currently not support saving as json";
    }

    virtual void SaveText(string file) override
    {
      LoadSave::SaveText(file);
      SaveText_(file);
      SaveText_NormalizerAndCalibrator(_saveNormalizerText, _saveCalibratorText);
    }

    virtual void SaveText_(string file)
    {
      LOG(WARNING) << Name() << " currently not support saving text format! Try SaveJson or SaveXml";
    }

    virtual void SaveCode_(string file, CodeType codeType)
    {
      LOG(WARNING) << Name() << " currently not support saving as code";
    }

    void SaveXml()
    {
      SaveXml(_path + "/model.xml");
    }

    void SaveJson()
    {
      SaveJson(_path + "/model.json");
    }

    void SaveText()
    {
      SaveText(_path + "/model.txt");
    }

    virtual void SaveFeaturesGain(int topNum = 0)
    {
      gezi::Noticer noticer("SaveFeaturesGain", 0);
      write_file(ToFeaturesGainSummary(topNum), _path + "/model.featureGain.txt");
    }

    virtual bool CustomLoad_(string path)
    {
      LOG(FATAL) << Name() << " currently has no custom load!";
      return false;
    }

    virtual bool LoadText_(string file)
    {
      LOG(FATAL) << Name() << " currently not support loading text format!";
      return false;
    }

    virtual bool LoadText(string path) override
    {
      ReadPathAndSetParam_(path);
      LoadSave::LoadText(path);
      string modelFile = endswith(path, ".txt") ? path : path + "/model.txt";
      bool ret = true;
      ret = LoadText_(modelFile);
      if (!ret)
        return ret;
      ret = LoadNormalizerAndCalibrator(path);
      return ret;
    }

    //This will result 
    //C++ signature :
    //std::shared_ptr < gezi::Normalizer > { lvalue } GetNormalizer(gezi::Predictor{ lvalue })
    //TypeError: No Python class registered for C++ class std::shared_ptr<gezi::Normalizer>
    //NormalizerPtr& GetNormalizer() 
    NormalizerPtr GetNormalizer()
    {
      return _normalizer;
    }

    void SetNormalizer(NormalizerPtr normalizer)
    {
      _normalizer = normalizer;
    }

    void SetCalibrator(CalibratorPtr calibrator)
    {
      _calibrator = calibrator;
    }

    CalibratorPtr& GetCalibrator()
    {
      return _calibrator;
    }

    Predictor& SetSaveNormalizerText(bool setSave = true)
    {
      _saveNormalizerText = setSave;
      return *this;
    }

    Predictor& SetSaveCalibratorText(bool setSave = true)
    {
      _saveCalibratorText = setSave;
      return *this;
    }

    static string ThirdModelName(string file)
    {
      return boost::replace_last_copy(file, ".bin", ".extra");
    }


    static bool& loadNormalizerAndCalibrator()
    {
      static bool _loadNormalizerAndCalibrator = true;
      return _loadNormalizerAndCalibrator;
    }

    static void SetLoadNormalizerAndCalibrator(bool loadNormalizerAndCalibrator_ = true)
    {
      loadNormalizerAndCalibrator() = loadNormalizerAndCalibrator_;
    }

    //-Utils
    //----------特征重要度 
    //单次预测特征重要度打印, 默认是按照gain的绝对值排序，如果sortByAbsGain = false，则按照feature id顺序打印
    virtual string ToGainSummary(Vector& features, bool sortByAbsGain = true)
    {
      return "";
    }

    //7863772 @FIXME大规模特征比如786w的时候index_sort类似hang住还是非常慢未知。。 
    //应该是index_sort有bug。。。
    template<typename Vec>
    string ToFeaturesGainSummary_(const Vec& gains, int topNum = 0)
    {
      typedef typename Vec::value_type value_type;
      int maxLen = (topNum == 0 || topNum > gains.size()) ? gains.size() : topNum;
      ivec indexVec = gezi::index_sort(gains, [](value_type l, value_type r) { return abs(l) > abs(r); }, maxLen);
      stringstream ss;
      for (int i = 0; i < maxLen; i++)
      {
        int idx = indexVec[i];
        ss << setiosflags(ios::left) << setfill(' ') << setw(100)
          << STR(i) + ":" + _featureNames[idx]
          << gains[idx] << endl;
      }
      return ss.str();
    }
    //整个模型中特征重要度的打印
    virtual string ToFeaturesGainSummary(int topNum = 0)
    {
      TryLoadFeatureNamesFromDefaultTextFile();
      return ToFeaturesGainSummary_(_featureGainVec, topNum);
    }

    void SetFeatureNames(const FeatureNamesVector& featureNames)
    {
      _featureNames = featureNames;
    }

    void SetFeatureGainVec(const vector<Float>& featureGainVec)
    {
      _featureGainVec = featureGainVec;
    }

    const FeatureNamesVector& FeatureNames() const
    {
      return _featureNames;
    }

    //可以避免风险 不提供& 如果必须显示通过move()调用下面的&& set一般两种语义 要么copy传值 要么转移传值 与传统的引用modify有区别
    //void SetFeatureGainVec(vector<Float>& featureGainVec)
    //{
    //	_featureGainVec = move(featureGainVec);
    //}

    void SetFeatureGainVec(vector<Float>&& featureGainVec)
    {
      _featureGainVec = move(featureGainVec);
    }

    friend class cereal::access;
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version)
    {
      auto& numFeatures = _featureNames._numFeatures;
      ar & CEREAL_NVP(numFeatures);
      auto& featureNames = *_featureNames._featureNames;
      ar & CEREAL_NVP(featureNames);
      //ar & CEREAL_NVP(_featureNames);
    }

  protected:
    friend class EnsemblePredictor;
    virtual Float Margin(Vector& features)
    {
      return 0;
    }

  private:
    void SaveText_NormalizerAndCalibrator(bool saveNormalizer = true, bool saveCalibrator = true)
    {
      if (saveNormalizer)
      {
        SAVE_SHARED_PTR_ASTEXT(_normalizer, _path);
      }
      if (saveCalibrator)
      {
        SAVE_SHARED_PTR_ASTEXT(_calibrator, _path);
      }
    }

    void SaveXml_NormalizerAndCalibrator(bool saveNormalizer = true, bool saveCalibrator = true)
    {
      if (saveNormalizer)
      {
        SAVE_SHARED_PTR_ASXML(_normalizer, _path);
      }
      if (saveCalibrator)
      {
        SAVE_SHARED_PTR_ASXML(_calibrator, _path);
      }
    }

    void SaveJson_NormalizerAndCalibrator(bool saveNormalizer = true, bool saveCalibrator = true)
    {
      if (saveNormalizer)
      {
        SAVE_SHARED_PTR_ASJSON(_normalizer, _path);
      }
      if (saveCalibrator)
      {
        SAVE_SHARED_PTR_ASJSON(_calibrator, _path);
      }
    }

  protected:
    void TryLoadFeatureNamesFromDefaultTextFile(string featureNameFile="./feature_name.txt")
    {
      if (_featureNames.NumFeatureNames() == 0)
      {
        VLOG(0) << "Try load feature names from " << featureNameFile;
        _featureNames.Load(featureNameFile);
        Pval_1(_featureNames.NumFeatures());
        Pval_1(_featureNames.NumFeatureNames());
      }
    }

  protected:
    bool _normalizeCopy = false;
    NormalizerPtr _normalizer = nullptr;
    CalibratorPtr _calibrator = nullptr; //for classification

    //svec _featureNames;
    FeatureNamesVector _featureNames;

    string _path;
    string _param;
    vector<Float> _featureGainVec;
  private:
    bool _saveNormalizerText = false; //是否输出文本格式 包括xml,json统一使用
    bool _saveCalibratorText = false;

    bool _useCustomModel = false;
  };

  //表示输出的本身就是概率 可以不用calibrator
  class ProbabilityPredictor : public Predictor
  {
  public:
    using Predictor::Predict;
    virtual Float Predict(Float output) override
    {
      if (_calibrator != nullptr)
      {
        return _calibrator->PredictProbability(output);
      }
      else
      {
        return output;
      }
    }
  };

  typedef shared_ptr<Predictor> PredictorPtr;
  typedef vector<PredictorPtr> Predictors;
}  //----end of namespace gezi

#endif  //----end of M_L_CORE__PREDICTOR_H_
