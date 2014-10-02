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

#include "Prediction/Normalization/NormalizerFactory.h"
#include "Prediction/Calibrate/CalibratorFactory.h"
#include "Prediction/Instances/Instance.h"
#include "Prediction/Normalization/Normalizer.h"
#include "Prediction/Calibrate/Calibrator.h"
namespace gezi {

	//��ǰֻ�ǿ��� ������ģ��
	//@TODO �����ģ��֧�� ���� Fvec ����Predict����Fvec ?, Regression Rank��ģ�͵�֧��
	//@TODO TLC c#�����ֽӿڵ���� �ܹ�Ǩ�Ƶ�c++��
	//var stronglyTypedPredictor = predictor as IPredictor<Instance, float>;
	class Predictor : public LoadSave
	{
	public:
		Predictor() = default;
		virtual ~Predictor() {}
		Predictor(NormalizerPtr normalizer, CalibratorPtr calibrator, const svec& featureNames)
			:_normalizer(normalizer), _calibrator(calibrator), _featureNames(featureNames),
			_numFeatures(_featureNames.size())
		{

		}

		Predictor(NormalizerPtr normalizer, CalibratorPtr calibrator, svec&& featureNames)
			:_normalizer(normalizer), _calibrator(calibrator), _featureNames(featureNames),
			_numFeatures(_featureNames.size())
		{

		}

		virtual string Name() override
		{
			return "Predictor";
		}

		string Path()
		{
			return _path;
		}

		void SetParam(string param)
		{
			_param = param;
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
		Float Output(InstancePtr instance)
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

		Float Predict(InstancePtr instance)
		{
			return Predict(instance->features);
		}

		Float Predict(Vector& features)
		{
			return Predict(Output(features));
		}

		Float Predict(Instance& instance, Float& output)
		{
			return Predict(instance.features, output);
		}

		Float Predict(InstancePtr instance, Float& output)
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
			{
				return 1.0 / (1.0 + exp(-output));
			}
		}

		void SetNormalizeCopy(bool normalizeCopy = true)
		{
			_normalizeCopy = normalizeCopy;
		}

		virtual void Save(string path) override
		{
			ChangeForSave();
			LoadSave::Save(path);
			_path = path;
			try_create_dir(path);
			string modelFile = path + "/model";
			Save_(modelFile);
			write_file(Name(), path + "/model.name.txt");
			write_file(_param, path + "/model.param.txt");
			SAVE_SHARED_PTR(_normalizer); //@TODO ��ʱshared ptr�Զ����л�û��Ū�ɹ� ���Ե����ֶ�����save Ҳ��̫�鷳
			SAVE_SHARED_PTR(_calibrator);
			ChangeForLoad();
		}

		virtual void Save_(string file) 
		{
			VLOG(0) << Name() << " currently not support saving model";
		}

		virtual void Load(string path) override
		{
			ChangeForLoad();
			_path = path;
			_param = read_file(OBJ_NAME_PATH(_param));
			LoadSave::Load(path);
			string modelFile = path + "/model";
			Load_(modelFile);
			string normalizerName = read_file(OBJ_NAME_PATH(_normalizer));
			if (!normalizerName.empty())
			{
				_normalizer = NormalizerFactory::CreateNormalizer(normalizerName, OBJ_PATH(_normalizer));
			}
			string calibratorName = read_file(OBJ_NAME_PATH(_calibrator));
			if (!calibratorName.empty())
			{
				_calibrator = CalibratorFactory::CreateCalibrator(calibratorName, OBJ_PATH(_calibrator));
			}
		}

		virtual void Load_(string file) 
		{
			VLOG(0) << Name() << " currently not support loading model";
		}

		//ע�ⲻ����save xml����save text ����Ҫ����ʹ�� ���� ����Save Ҳ���Ǵ洢������֮��ʹ��
		//xml��text���ṩ���� Ŀǰ ���ǰ��ն��������� ����������Ŀ�Դ�������linear model��ʽ������תLinearPredictorȻ��
		//Save 
		virtual void SaveXml(string file) override
		{
			ChangeForSave();
			LoadSave::SaveXml(file);
			SaveXml_(file);
			SAVE_SHARED_PTR_ASXML(_calibrator); //@TODO calibrator, normalizer֧��xml
			SAVE_SHARED_PTR_ASXML(_normalizer);
			ChangeForLoad();
		}

		virtual void SaveXml_(string file) 
		{
			VLOG(0) << Name() << " currently not support saving xml";
		}

		virtual void SaveJson(string file) override
		{
			ChangeForSave();
			LoadSave::SaveJson(file);
			SaveJson_(file);
			SAVE_SHARED_PTR_ASJSON(_calibrator); //@TODO calibrator, normalizer֧��xml
			SAVE_SHARED_PTR_ASJSON(_normalizer);
			ChangeForLoad();
		}

		virtual void SaveJson_(string file)
		{
			VLOG(0) << Name() << " currently not support saving json";
		}

		virtual void SaveText(string file) override
		{
			//Ĭ���ǲ�֧��text��ʽд���� �����Լ�������д��ģ����� һ��Ϊ��debug���߸���չʾmodel��Ϣ
			//һ�������ѡ��д��xml�Ϳ��ԽϺÿ���ģ����Ϣ�� ����Ҫ��д prefer SaveXML
			SaveText_(file);
			SaveText_NormalizerAndCalibrator(_saveNormalizerText, _saveCalibratorText);
		}

		virtual void SaveText_(string file) 
		{
			VLOG(0) << Name() << " currently not support saving text format! Try SaveXml";
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

		NormalizerPtr GetNormalizer()
		{
			return _normalizer;
		}

		CalibratorPtr GetCalibrator()
		{
			return _calibrator;
		}

		void SetNumFeatures(int numFeatures)
		{
			_numFeatures = numFeatures;
		}

		int GetNumFeatures()
		{
			return _numFeatures;
		}

		void SetUnSaveNormalizerText()
		{
			_saveNormalizerText = false;
		}

		void SetUnSaveCalibratorText()
		{
			_saveCalibratorText = false;
		}

		friend class cereal::access;
		template<class Archive>
		void serialize(Archive &ar, const unsigned int version)
		{
			ar & CEREAL_NVP(_numFeatures);
			//@TODO �����������������������Ŀ�޴� ��һ��ʱ���˷�
			//����������Ƿ��ӡ������֣� �������⴦��f0 f1���ֲ��������ʱ������л� ����ʱ�����л��Զ��ָ���f0,f1������
			ar & CEREAL_NVP(_featureNames);
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
				SAVE_SHARED_PTR_ASTEXT(_normalizer);
			}
			if (saveCalibrator)
			{
				SAVE_SHARED_PTR_ASTEXT(_calibrator);
			}
		}
	
		void ChangeForSave()
		{
			if (_numFeatures > 1000 && _featureNames[0] == "f0" && _featureNames[999] == "f999")
			{
				_featureNames.clear();
			}
		}
	protected:
		void ChangeForLoad()
		{
			if (_numFeatures > _featureNames.size())
			{
				CreateFakeFeatureNames();
			}
		}

		void CreateFakeFeatureNames()
		{
			for (int i = 0; i < _numFeatures; i++)
			{
				_featureNames.push_back("f" + STR(i));
			}
		}
	protected:
		bool _normalizeCopy = false;
		NormalizerPtr _normalizer = nullptr;
		CalibratorPtr _calibrator = nullptr;

		svec _featureNames;
		int _numFeatures = 0; //@TODO maybe uint int64 .. ��ʱֻ����int��Χ�ڵ����� �ı�����᲻�ᳬ������

		string _path;
		string _param;

	private:
		bool _saveNormalizerText = true;
		bool _saveCalibratorText = true;
	};
	typedef shared_ptr<Predictor> PredictorPtr;
	typedef vector<PredictorPtr> Predictors;
}  //----end of namespace gezi

#endif  //----end of M_L_CORE__PREDICTOR_H_
