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

	//��ǰֻ�ǿ��� ������ģ��
	//@TODO �����ģ��֧�� ���� Fvec ����Predict����Fvec ?, Regression Rank��ģ�͵�֧��
	//@TODO TLC c#�����ֽӿڵ���� �ܹ�Ǩ�Ƶ�c++�� c#�Ĺؼ���һ��  object Predict(object features);   public interface IPredictor<in TFeatures, out TResult> : IPredictor
	//TResult Predict(TFeatures features);
	//var stronglyTypedPredictor = predictor as IPredictor<Instance, float>;
	//dvec Predicts(const Vector&) 
	//double Predict(const Vector&, dvec&)
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

		string GetParam()
		{
			return _param;
		}

		//���δ��Calibrator��������ֵ -n,+n 0��ʾ�ֽ� Խ��Խ����positive
		Float Output(Instance& instance)
		{
			return Output(instance.features);
		}

		//��Melt.h ͳһʹ���麯���ӿ�  ������ InstancePtr
		virtual Float Output(InstancePtr instance)
		{
			return Output(instance->features);
		}

		void Print(const Vector& features, std::ostream& ofs = std::cout)
		{
			features.ForEachNonZero([this, &ofs](int index, Float value)
			{
				ofs << index << "\t" << _featureNames[index] << "\t" << value << std::endl;
			});
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

		//@TODO ���Ǿ����� ���� const �������������������
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

		//�����Calibrator��������ֵ [0,1], ���ֵ��ʾ�������positive�ĸ���
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

		virtual Float Output(string line)
		{
			return 0.0;
		}

		virtual Float Predict(string line, Float& output)
		{
			return 0.0;
		}

		virtual Float Predict(InstancePtr instance)
		{
			return Predict(instance->features);
		}

		//@TODO  �ṩconst �汾
		Float Predict(Vector& features)
		{
			if (GetPredictionKind() == PredictionKind::Regression
				|| GetPredictionKind() == PredictionKind::MultiOutputRegression)
			{
				return Output(features);
			}
			else
			{
				return Predict(Output(features));
			}
		}

		Float Predict(Instance& instance, Float& output)
		{
			return Predict(instance.features, output);
		}

		virtual Float Predict(InstancePtr instance, Float& output)
		{
			return Predict(instance->features, output);
		}

		Float Predict(Vector& features, Float& output)
		{
			output = Output(features);
			return Predict(output);
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

		Float Predict(const map<int, double>& m, Float& output)
		{
			Vector features(m);
			return Predict(features, output);
		}

		Float Predict(Float output)
		{
			if (_calibrator != nullptr)
			{
				return _calibrator->PredictProbability(output);
			}
			else
			{ //f(x) = (1/2) * log(pr(true) / 1 - pr(true))
				//@TODO ����fastrank���Ż�Ŀ�꺯��output->prob�Ƶ� Ӧ���� -2*output �����ƺ������� -output��Ӧ��loglossҪ�� check �Ƿ�ͬ���ݼ��ϲ�һ��
				//return 1.0 / (1.0 + exp(-2.0 * output));
				//return 1.0 / (1.0 + exp(-output)); // == gezi::sigmoid(output)
				//����ע��ϴ��ֵ���ܻᶼӳ�䵽0����1�Ӷ���ʧ������Ϣ,auc����
				return gezi::sigmoid(output);
			}
		}

		Predictor& SetNormalizeCopy(bool normalizeCopy = true)
		{
			_normalizeCopy = normalizeCopy;
			return *this;
		}

		virtual void Save()
		{
			Save(_path);
		}

		virtual void Save(string path) override
		{
			LoadSave::Save(path);
			_path = path;
			try_create_dir(path);
			string modelFile = path + "/model.bin";
			Save_(modelFile);
			write_file(Name(), path + "/model.name.txt");
			write_file(_param, path + "/model.param.txt");
			SAVE_SHARED_PTR(_normalizer, path);
			SAVE_SHARED_PTR(_calibrator, path);
		}

		virtual void Save_(string file)
		{
			LOG(WARNING) << Name() << " currently not support saving model";
		}

		void LoadNormalizerAndCalibrator(string path)
		{
			if (loadNormalizerAndCalibrator())
			{
#ifdef NO_CEREAL
				string normalizerName = read_file(OBJ_NAME_PATH(_normalizer, path));
				if (!normalizerName.empty())
				{ //����������cereal���л���̬shared ptr���� ������Ҫ������ͨ��·������������� �������Ĭ��·��
					_normalizer = NormalizerFactory::CreateNormalizer(normalizerName, OBJ_PATH(_normalizer, path));
				}
				string calibratorName = read_file(OBJ_NAME_PATH(_calibrator, path));
				if (!calibratorName.empty())
				{
					_calibrator = CalibratorFactory::CreateCalibrator(calibratorName, OBJ_PATH(_calibrator, path));
				}
#else 
				serialize_util::load(_normalizer, OBJ_PATH(_normalizer, path));
				serialize_util::load(_calibrator, OBJ_PATH(_calibrator, path));
#endif
			}
		}

		virtual void Load(string path) override
		{
			_path = path;
			_param = read_file(OBJ_NAME_PATH(_param, path));
			LoadSave::Load(path);
			string modelFile = path + "/model.bin";
			Load_(modelFile);
			LoadNormalizerAndCalibrator(path);
		}

		virtual void Load_(string file)
		{
			LOG(FATAL) << Name() << " currently not support loading model";
		}

		//ע�ⲻ����save xml����save text ����Ҫ����ʹ�� ���� ����Save Ҳ���Ǵ洢������֮��ʹ��
		//xml��text���ṩ���� Ŀǰ ���ǰ��ն��������� ����������Ŀ�Դ�������linear model��ʽ������תLinearPredictorȻ��
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
		//�������Ƶ�Ҳ����Ҫ��Save(path) �趨·�� Ҳ����˵�����ڴ洢bin model֮��,�洢bin�Ǳ���,�洢code��optional
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

		void SaveFeaturesGain(int topNum = 0)
		{
			write_file(ToFeaturesGainSummary(topNum), _path + "/model.featureGain");
		}

		virtual void LoadText_(string file)
		{
			LOG(FATAL) << Name() << " currently not support loading text format!";
		}

		virtual void LoadText(string path) override
		{
			_path = path;
			_param = read_file(OBJ_NAME_PATH(_param, _path));
			LoadSave::LoadText(path);
			string modelFile = path + "/model.txt";
			LoadText_(modelFile);
			LoadNormalizerAndCalibrator(path);
		}

		NormalizerPtr& GetNormalizer()
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

		//-Utils
		//----------������Ҫ�� 
		//����Ԥ��������Ҫ�ȴ�ӡ
		virtual string ToGainSummary(Vector& features)
		{
			return "";
		}

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
					<< STR(i) + STR(":") + _featureNames[idx]
					<< gains[idx] << endl;
			}
			return ss.str();
		}
		//����ģ����������Ҫ�ȵĴ�ӡ
		virtual string ToFeaturesGainSummary(int topNum = 0)
		{
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

		//���Ա������ ���ṩ& ���������ʾͨ��move()���������&& setһ���������� Ҫôcopy��ֵ Ҫôת�ƴ�ֵ �봫ͳ������modify������
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
		bool _normalizeCopy = false;
		NormalizerPtr _normalizer = nullptr;
		CalibratorPtr _calibrator = nullptr; //for classification

		//svec _featureNames;
		FeatureNamesVector _featureNames;

		string _path;
		string _param;
		vector<Float> _featureGainVec;
	private:
		bool _saveNormalizerText = false; //�Ƿ�����ı���ʽ ����xml,jsonͳһʹ��
		bool _saveCalibratorText = false;
	};

	typedef shared_ptr<Predictor> PredictorPtr;
	typedef vector<PredictorPtr> Predictors;
}  //----end of namespace gezi

#endif  //----end of M_L_CORE__PREDICTOR_H_
