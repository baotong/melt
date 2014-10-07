/**
 *  ==============================================================================
 *
 *          \file   Run/Melt.h
 *
 *        \author   chenghuige
 *
 *          \date   2014-04-01 11:51:11.160085
 *
 *  \Description: ����ѧϰ���߿�melt �����RunExperiments ��Ӧmelt.cc
 *  @TODO cmmand���� �ű�ͳһ�� ����ԭ���ͼ������FLAGSͬʱ����
 *  @TODO ���ݷָ�  train  test // validate ?
 *  @TODO ����shrink  ȥ���ض���pos ���� �ض��� neg ʹ�� ���������ﵽԤ����� 1:1 1:2 ��������� Ȼ����������index ˳�򼴿� done
 *  ��ʱ ��֧��missing feature, class feature, text feature
 * ����ֻʵ��
 * 1. ������� done
 * 2. Instances ���ݽṹ ϡ�� Dense �Զ�ת��  Done
 * 3. Normalization ��ǰʵ���� MinMax @TODO Gussian and Bin  done
 * 4. ѵ��֧�� Cross Fold ���cross ��ʵ�� , test ��δʵ��
 *    ����ʵ�������� train, test, train-test, cross fold
 * 5. @TODO ����evaluatore ������� �ж�Ч��
 * 6. @TODO ����ѡ��  grid sweeping ����ʵ�� ��������ѡ�� ?
 * 7. ����ʵ��Binary���� �����ٿ�������
 * 8. ����ʵ�ֶ������ LinearSvm   FastRank -> @TODO �߼��ع�, KernelSvm,LibLinear, ���ɭ�֡�����
 * 9. ��ӡfeature���� ��Сֵ ���ֵ ��ֵ ���� ��������� ���������  ��������������index ��ȡ���
 *  ==============================================================================
 */

#ifndef RUN__MELT_H_
#define RUN__MELT_H_
#define NO_BAIDU_DEP
#include "common_util.h"
#include "Run/MeltArguments.h"
#include "Prediction/Instances/InstanceParser.h"
#include "Run/CVFoldCreator.h"
#include "Prediction/Normalization/MinMaxNormalizer.h"
#include "Prediction/Normalization/NormalizerFactory.h"
#include "Prediction/Calibrate/CalibratorFactory.h"
#include "Utils/FeatureStatus.h"
#include "Prediction/Instances/instances_util.h"
#include "MLCore/TrainerFactory.h"

#include "MLCore/Predictor.h"

#include "Prediction/Instances/instances_util.h"
#include "MLCore/PredictorFactory.h"

#include "Utils/performance_evaluate.h"

namespace gezi {
	class Melt
	{
	public:
		Melt()
		{
			ParseArguments();
			Pval(_cmd.randSeed);
		}
		~Melt()
		{
		}
		void ParseArguments();

		enum class RunType
		{
			UNKNOWN = 0,
			HELP,
			EVAL, //������֤,Ĭ��ִ�е�command
			EVAL_PARAM, //������֤ ����ֻ���auc��ֵ,��Ҫ���ڼ�ⲻͬ����Ч���Ա�
			TRAIN, //ѵ��
			TEST,  //����
			TRAIN_TEST,  //ѵ��+����
			FEATURE_SELECTION,  //����ѡ��  //����ѡ��ŵ���Χpython�ű� 
			CREATE_INSTANCES,  //��������normal��ʽ������ת��Ϊ���ϵ�,����catogry,text���չ��
			NORMALIZE,  //���й�һ�������������ı�,��normalizer��Ϣ���л���-m��Ӧ��·����
			CALIBRATE,  //����output->[0-1]�Ĺ�һ������calibrator��Ϣ���л���-m��Ӧ��·����
			CHECK_DATA, //������� ��ǰ�ǽ���minmax��һ ��ʾ��������
			FEATURE_STATUS, //��ӡ�����ľ�ֵ������Ϣ��mean var
			SHOW_FEATURES, //��ӡ������
			SHOW_INFOS, //չʾ�������ݵĻ�����Ϣ  ������Ŀ��������Ŀ����������
			CONVERT, //�����������ļ�����Ȼ�����Ҫ��ĸ�ʽ ���� dense -> sparse
			SPLIT_DATA, //���������������з�  ���� 1:1 1:3:2 ͬʱ����ÿ��������������ά�ֺ�ԭʼ����һ��
			GEN_CROSS_DATA, //���������֤���ı������ļ� ����Ա���������ѧϰ���ߵ�ʵ��Ч��
			CHANGE_RAIO, //���������������������������� ���� ԭʼ 1:30 ����Ϊ 1:1
			WRITE_TEXT_MODEL, //����binaryģ�ͺ�д���ı���ʽģ����-mt -mxml -mjson ע��ģ���ڲ���normalizer,calibratorĬ�϶��ǲ����Text��ʽ�ģ������Ҫ��� -snt 1, -sct 1
			TEXT_MODEL_TO_BINARY //��ȡ-m ָ��·���µ�model.txt �û�ָ��ģ������-cl Ĭ����LinearSVM �����ı���ʽģ������д��binaryģ�͵�-m·��
		};

		enum class CrossValidationType
		{
			DEFAULT = 0, //Ĭ������instance�ļ��������ⲿpython�ű�����instance�ļ�����evaluate���
			AUC = 1 //������instance�ļ�,ֻ���ڲ�����auc��������evaluate����@TODO,���ڲ���ѡȡ
		};

		MeltArguments& Cmd()
		{
			return _cmd;
		}

		void PrintCommands()
		{
			VLOG(0) << "Supported commands now are below: [commandName or commandShortName] <-> [commandId]";
			for (auto item : _commands)
			{
				VLOG(0) << setiosflags(ios::left) << setfill(' ') << setw(40)
					<< item.first << " " << (int)item.second;
			}
			int i = 2; // 0 UNKNOWN, 1 HELP
			VLOG(0) << i++ << " EVAL, //������֤,Ĭ��ִ�е�command";
			VLOG(0) << i++ << " EVAL_PARAM, //������֤ ����ֻ���auc��ֵ,��Ҫ���ڼ�ⲻͬ����Ч���Ա�";
			VLOG(0) << i++ << " TRAIN, //ѵ��(-mt -mxml -mjson���ÿ��������Ӧ�ı���ʽģ�ͣ����Ҫ���ڲ���normalizer�����Ӧ�ı���ʽ���� -snt 1,calibrator���� -sct 1)";
			VLOG(0) << i++ << " TEST,  //����";
			VLOG(0) << i++ << " TRAIN_TEST,  //ѵ��+����";
			VLOG(0) << i++ << " FEATURE_SELECTION,  //����ѡ��  //����ѡ��ŵ���Χpython�ű�";
			VLOG(0) << i++ << " CREATE_INSTANCES,  //��������normal��ʽ������ת��Ϊ���ϵ�,����catogry,text���չ��";
			VLOG(0) << i++ << " NORMALIZE,  //���й�һ�������������ı�,��normalizer��Ϣ���л���-m��Ӧ��·����(-mt -mxml -mjson���ÿ��������Ӧ�ı���ʽ ����Ҫ����-snt)";
			VLOG(0) << i++ << " CALIBRATE,  //����output->[0-1]�Ĺ�һ������calibrator��Ϣ���л���-m��Ӧ��·����  -mt -mxml -mjson���ÿ��������Ӧ�ı���ʽ ����Ҫ����-sct";
			VLOG(0) << i++ << " CHECK_DATA, //������� ��ǰ�ǽ���minmax��һ ��ʾ��������";
			VLOG(0) << i++ << " FEATURE_STATUS, //��ӡ�����ľ�ֵ������Ϣ��mean var";
			VLOG(0) << i++ << " SHOW_FEATURES, //��ӡ������";
			VLOG(0) << i++ << " SHOW_INFOS, //չʾ�������ݵĻ�����Ϣ  ������Ŀ��������Ŀ����������";
			VLOG(0) << i++ << " CONVERT, //�����������ļ�����Ȼ�����Ҫ��ĸ�ʽ ���� dense -> sparse";
			VLOG(0) << i++ << " SPLIT_DATA, //���������������з�  ���� 1:1 1:3:2 ͬʱ����ÿ��������������ά�ֺ�ԭʼ����һ��";
			VLOG(0) << i++ << " GEN_CROSS_DATA, //���������֤���ı������ļ� ����Ա���������ѧϰ���ߵ�ʵ��Ч��";
			VLOG(0) << i++ << " CHANGE_RAIO //���������������������������� ���� ԭʼ 1:30 ����Ϊ 1:1";
			VLOG(0) << i++ << " WRITE_TEXT_MODEL // ����binaryģ�ͺ�д���ı���ʽģ����-mt -mxml -mjson(ע���ڲ���normalizer�����Ҫ�ı������Ҫ-snt 1,���Ƶ�calibrator�ı���� -sct 1)";
			VLOG(0) << i++ << " TEXT_MODEL_TO_BINARY //��ȡ-m ָ��·���µ�model.txt �û�ָ��ģ������-cl Ĭ����LinearSVM �����ı���ʽģ������д��binaryģ�͵�-m·��";
		}

		static string GetOutputFileName(string infile, string suffix, bool removeTxt = false)
		{
			if (!removeTxt)
				return endswith(infile, ".txt") ? boost::replace_last_copy(infile, ".txt", "." + suffix + ".txt") : infile + "." + suffix;
			else
				return endswith(infile, ".txt") ? boost::replace_last_copy(infile, ".txt", "." + suffix) : infile + "." + suffix;
		}

		void RunCrossValidation(Instances& instances, CrossValidationType cvType)
		{
			//--------------------------- ����ļ�ͷ
			string instFile = _cmd.resultDir + "/" + STR(_cmd.resultIndex) + ".inst.txt";
			ofstream ofs;
			if (cvType == CrossValidationType::DEFAULT)
			{
				try_create_dir(_cmd.resultDir);
				ofs.open(instFile);
				WriteInstFileHeader(ofs);
			}

			if (_cmd.preNormalize)
			{
				NormalizerPtr normalizer = NormalizerFactory::CreateNormalizer(_cmd.normalizerName);
				CHECK(normalizer != nullptr);
				Pval(normalizer->Name());
				normalizer->RunNormalize(instances);
			}
			const int randomStep = 10000;
			//const int randomStep = 1;
			BinaryClassficationEvaluatorPtr evaluator = nullptr;
			if (cvType == CrossValidationType::AUC)
			{
				evaluator = make_shared<AucEvaluator>();
			}
			string trainerParam;
			for (size_t runIdx = 0; runIdx < _cmd.numRuns; runIdx++)
			{
				VLOG(0) << "The " << runIdx << " round";
				RandomEngine rng = random_engine(_cmd.randSeed, runIdx * randomStep);
				if (!_cmd.foldsSequential)
					instances.Randomize(rng);

				ivec instanceFoldIndices = CVFoldCreator::CreateFoldIndices(instances, _cmd, rng);
				for (size_t foldIdx = 0; foldIdx < _cmd.numFolds; foldIdx++)
				{
					VLOG(0) << "Cross validaion foldIdx " << foldIdx;
					string instfile = format("{}/{}_{}_{}.inst.txt", _cmd.resultDir, _cmd.resultIndex
						,runIdx, foldIdx);

					Instances trainData, testData;
					//ֻ��trainProportion < 1 ����Ҫrng
					CVFoldCreator::CreateFolds(instances, _cmd.trainProportion,
						instanceFoldIndices, foldIdx, _cmd.numFolds, trainData, testData,
						random_engine(_cmd.randSeed, runIdx * randomStep));

					//------------------------------------Train
					TrainerPtr trainer = TrainerFactory::CreateTrainer(_cmd.classifierName);
					if (foldIdx == 0)
					{
						VLOG(0) << "Folds " << foldIdx << " are trained with " << trainData.Size() << " instances, and tested on " << testData.Size() << " instances";
						Pval3(trainData[0]->name, trainData.PositiveCount(), trainData.NegativeCount());
						Pval3(testData[0]->name, testData.PositiveCount(), testData.NegativeCount());
					}

					trainer->SetNormalizeCopy();
					trainer->Train(trainData);
					PredictorPtr predictor = trainer->CreatePredictor();
					predictor->SetNormalizeCopy();

					if (cvType == CrossValidationType::DEFAULT)
					{
						//@TODO ÿ��test ���һ��inst �ļ�Ҳ Ȼ��ÿ������һ�����
						VLOG(0) << "-------------------------------------Testing";
						Test(testData, predictor, instfile, ofs);
						string command = _cmd.evaluate + instfile;
#pragma omp critical
						{
							EXECUTE(command);
						}
					}
					else if (cvType == CrossValidationType::AUC)
					{
						Test(testData, predictor, evaluator);
					}
					else
					{
						LOG(ERROR) << "Not supported crosss validation type";
						return;
					}
					if (foldIdx == 0)
					{
						trainerParam = trainer->GetParam();
					}
				}
			}

			if (cvType == CrossValidationType::DEFAULT)
			{
				string command = _cmd.evaluate + instFile;
#pragma omp critical
				{
					EXECUTE(command);
				}
			}
			else if (cvType == CrossValidationType::AUC)
			{
				double aucScore = evaluator->Finish();
				cout << "aucScore: " << aucScore << "\t" << "trainerParam: " << trainerParam << endl;
			}
		}

		void RunCrossValidation(CrossValidationType cvType = CrossValidationType::DEFAULT)
		{
			Noticer nt(format("{} fold cross-validation", _cmd.numFolds));
			//----------------------------check if command ok
			CHECK_GE(_cmd.numFolds, 2) << "The number of folds must be at least 2 for cross validation";
			//-----------------------------parse input
			Instances instances = create_instances(_cmd.datafile);
			CHECK_GT(instances.Count(), 0) << "Read 0 instances, aborting experiment";
			instances.PrintSummary();
			//------------------------------run
			RunCrossValidation(instances, cvType);
		}

		void WriteInstFileHeader(ofstream& ofs)
		{
			ofs << "Instance\tTrue\tAssigned\tOutput\tProbability" << endl;
		}

		void Test(Instances& instances, PredictorPtr predictor,
			string outfile, ofstream& ofs)
		{
			//@TODO ����дÿ��round�ĵ����ļ� ����c++�汾�ڲ���evaluator�������չʾ
			ofstream currentOfs(outfile);
			WriteInstFileHeader(currentOfs);
			Test(instances, predictor, ofs, currentOfs);
		}

		void Test(Instances& instances, PredictorPtr predictor, string outfile)
		{
			ofstream ofs(outfile);
			WriteInstFileHeader(ofs);
			Test(instances, predictor, ofs);
		}

		void Test(Instances& instances, PredictorPtr predictor, ofstream& ofs)
		{
			int idx = 0;
			ProgressBar pb(instances.Count(), "Testing");
			for (InstancePtr instance : instances)
			{
				++pb;
				double output;
				double probability = predictor->Predict(instance, output);
				string name = instance->name.empty() ? STR(idx) : instance->name;
				if (startswith(name, '_'))
				{
					name = name.substr(1);
				}

				int assigned = output > 0 ? 1 : 0;
				ofs << name << "\t" << instance->label << "\t"
					<< assigned << "\t" << output << "\t"
					<< probability << endl;
				VLOG(6) << name << "\t" << instance->label << "\t"
					<< assigned << "\t" << output << "\t"
					<< probability;
				idx++;
			}
		}

		void Test(Instances& instances, PredictorPtr predictor, ofstream& ofs, ofstream& currentOfs)
		{
			int idx = 0;
			ProgressBar pb(instances.Count(), "Testing");
			for (InstancePtr instance : instances)
			{
				++pb;
				double output;
				double probability = predictor->Predict(instance, output);
				string name = instance->name.empty() ? STR(idx) : instance->name;
				if (startswith(name, '_'))
				{
					name = name.substr(1);
				}

				int assigned = output > 0 ? 1 : 0;
#pragma  omp critical
				{
					ofs << name << "\t" << instance->label << "\t"
						<< assigned << "\t" << output << "\t"
						<< probability << endl;
				}
				currentOfs << name << "\t" << instance->label << "\t"
					<< assigned << "\t" << output << "\t"
					<< probability << endl;
				VLOG(6) << name << "\t" << instance->label << "\t"
					<< assigned << "\t" << output << "\t"
					<< probability;
				idx++;
			}
		}

		string TestLazyStore(Instances& instances, PredictorPtr predictor)
		{
			stringstream ofs;
			int idx = 0;
			ProgressBar pb(instances.Count(), "Testing");
			for (InstancePtr instance : instances)
			{
				++pb;
				double output;
				double probability = predictor->Predict(instance, output);
				string name = instance->name.empty() ? STR(idx) : instance->name;
				if (startswith(name, '_'))
				{
					name = name.substr(1);
				}

				int assigned = output > 0 ? 1 : 0;
				ofs << name << "\t" << instance->label << "\t"
					<< assigned << "\t" << output << "\t"
					<< probability << endl;
				VLOG(6) << name << "\t" << instance->label << "\t"
					<< assigned << "\t" << output << "\t"
					<< probability;
				idx++;
			}
			return ofs.str();
		}


		//AUC test
		void Test(Instances& instances, PredictorPtr predictor,
			BinaryClassficationEvaluatorPtr evaluator)
		{
			for (InstancePtr instance : instances)
			{
				double probability = predictor->Predict(instance);
				evaluator->Add(instance->label, probability, instance->weight);
			}
		}

		PredictorPtr Train(Instances& instances)
		{
			Pval(_cmd.classifierName);
			auto trainer = TrainerFactory::CreateTrainer(_cmd.classifierName);
			if (trainer == nullptr)
			{
				LOG(WARNING) << _cmd.classifierName << " has not been supported yet";
				return;
			}
			trainer->Train(instances);
			auto predictor = trainer->CreatePredictor();
			predictor->SetParam(trainer->GetParam());
			return predictor;
		}

		void RunTrain()
		{
			PredictorPtr predictor;
			Instances instances;
			{
				Noticer nt("Train!");
				instances = create_instances(_cmd.datafile);
				CHECK_GT(instances.Count(), 0) << "Read 0 test instances, aborting experiment";
				predictor = Train(instances);
			}
			if (_cmd.selfTest)
			{
				Noticer nt("Test itself!");
				try_create_dir(_cmd.resultDir);
				string instFile = _cmd.resultDir + "/" + STR(_cmd.resultIndex) + ".inst.txt";
				//auto testInstances = create_instances(_cmd.datafile);
				auto& testInstances = instances; //����Train�Ĺ���û�иı�instances û��normlaize���ı��� @TODO ȷ��NormalizeCopy ?
				//could use deep copy of instances at first so do not need reload from disk and also avoid modification during train like normalize
				CHECK_GT(testInstances.Count(), 0) << "Read 0 test instances, aborting experiment";
				Test(testInstances, predictor, instFile);
				string command = _cmd.evaluate + instFile;
				EXECUTE(command);
			}
			{
				Noticer nt("Write train result!");

				(*predictor).SetSaveNormalizerText(_cmd.saveNormalizerText)
					.SetSaveCalibratorText(_cmd.saveCalibratorText);

				predictor->Save(_cmd.modelFolder);
				if (_cmd.modelfileXml)
				{
					predictor->SaveXml();
				}
				if (_cmd.modelfileJson)
				{
					predictor->SaveJson();
				}
				if (_cmd.modelfileText)
				{
					predictor->SaveText();
				}
			}
		}

		void RunTest()
		{
			Noticer nt("Test! with model from " + _cmd.modelFolder);
			//------load predictor
			PredictorPtr predictor;
			{
				Noticer nt("Loading predictor");
				predictor = PredictorFactory::LoadPredictor(_cmd.modelFolder);
			}
			//------test
			try_create_dir(_cmd.resultDir);
			string instFile = _cmd.resultDir + "/" + STR(_cmd.resultIndex) + ".inst.txt";

			//@TODO hack for text input format //Not tested correctness yet
			InstanceParser::TextFormatParsedTime(); //++ pared text from time������ʾ��Ҫʹ�ôʱ�����
			string testDatafile = _cmd.testDatafile.empty() ? _cmd.datafile : _cmd.testDatafile;
			auto testInstances = create_instances(testDatafile);
			CHECK_GT(testInstances.Count(), 0) << "Read 0 test instances, aborting experiment";
			Test(testInstances, predictor, instFile);
			string command = _cmd.evaluate + instFile;
			EXECUTE(command);
		}

		void RunTrainTest()
		{
			Noticer nt("TrainTest!");
			PredictorPtr predictor;
			Instances instances;
			{
				Noticer nt("Train!");
				instances = create_instances(_cmd.datafile);
				CHECK_GT(instances.Count(), 0) << "Read 0 train instances, aborting experiment";
				predictor = Train(instances);
			}
			{
				Noticer nt("Test!");
				try_create_dir(_cmd.resultDir);
				string instFile = _cmd.resultDir + "/" + STR(_cmd.resultIndex) + ".inst.txt";

				auto testInstances = create_instances(_cmd.testDatafile);
				CHECK_GT(testInstances.Count(), 0) << "Read 0 test instances, aborting experiment";
				CHECK_EQ(instances.schema == testInstances.schema, 1);
				Test(testInstances, predictor, instFile);
				string command = _cmd.evaluate + instFile;
				EXECUTE(command);
			}
			{
				Noticer nt("Write train result!");
				predictor->Save(_cmd.modelFolder);
				if (_cmd.modelfileXml)
				{
					predictor->SaveXml();
				}
				if (_cmd.modelfileJson)
				{
					predictor->SaveJson();
				}
				if (_cmd.modelfileText)
				{
					predictor->SaveText();
				}
			}
		}

		void RunFeatureSelection()
		{
			Noticer nt("FeatureSelection!");
			Instances instances = create_instances(_cmd.datafile);
		}

		void RunCreateInstances()
		{

		}

		void RunShowFeatures()
		{
			Instances instances = create_instances(_cmd.datafile);
			fmt::print("Num features: {}", instances.NumFeatures());
			int num = 0;
			for (string feature : instances.FeatureNames())
			{
				std::cout << num++ << "\t" << feature << endl;
			}
		}

#define  SAVE_SHARED_PTR_ALL(obj)\
		{\
		SAVE_SHARED_PTR(obj, _cmd.modelFolder); \
		if (_cmd.modelfileXml)\
		{\
		SAVE_SHARED_PTR_ASXML(obj, _cmd.modelFolder); \
		}\
		if (_cmd.modelfileJson)\
		{\
		SAVE_SHARED_PTR_ASJSON(obj, _cmd.modelFolder); \
		}\
		if (_cmd.modelfileText)\
		{\
		SAVE_SHARED_PTR_ASTEXT(obj, _cmd.modelFolder); \
		}\
		}

		void RunNormalize()
		{
			Noticer nt("Normalize!");
			NormalizerPtr normalizer = NormalizerFactory::CreateNormalizer(_cmd.normalizerName);
			CHECK(normalizer != nullptr);
			Pval(normalizer->Name());

			string infile = _cmd.datafile;
			string suffix = normalizer->Name() + ".normed";
			string outfile = _cmd.outfile.empty() ? GetOutputFileName(infile, suffix) : _cmd.outfile;
			Pval(outfile);

			Instances instances = create_instances(_cmd.datafile);

			normalizer->RunNormalize(instances);
			FileFormat fileFormat = get_value(kFormats, _cmd.outputFileFormat, FileFormat::Unknown);
			write(instances, outfile, fileFormat);

			try_create_dir(_cmd.modelFolder);
			SAVE_SHARED_PTR_ALL(normalizer);
		}

		void RunCalibrate()
		{
			Noticer nt("Calibrate!");

			auto calibrator = CalibratorFactory::CreateCalibrator(_cmd.calibratorName);
			CHECK(calibrator != nullptr);
			Pval(calibrator->Name());

			Instances instances = create_instances(_cmd.datafile);
			auto predictor = PredictorFactory::LoadPredictor(_cmd.modelFolder);
			calibrator->Train(instances, [&predictor](InstancePtr instance) { return predictor->Output(instance); });

			try_create_dir(_cmd.modelFolder);
			//@WARNING calibrator ������ֲ��ܱ� ����Ҫд��calibrator.bin... normalizer����
			SAVE_SHARED_PTR_ALL(calibrator);
		}

		void RunCheckData()
		{
			Noticer nt("CheckData!(need GLOG_v=4 ./melt)");
			Instances instances = create_instances(_cmd.datafile);
			NormalizerPtr normalizer = make_shared<MinMaxNormalizer>();
			normalizer->Prepare(instances);
		}

		void RunShowInfos()
		{
			auto instances = create_instances(_cmd.datafile); //�ڲ���ӡ��Ϣ
		}

		void RunFeatureStatus()
		{
			Noticer nt("FeatureStatus!");
			string infile = _cmd.datafile;
			string suffix = "featurestatus";
			string outfile = _cmd.outfile.empty() ? GetOutputFileName(infile, suffix)
				: _cmd.outfile;
			Instances instances = create_instances(_cmd.datafile);
			FeatureStatus::GenMeanVarInfo(instances, outfile, _cmd.featureName);
		}

		//�����ļ�ת�������
		void RunConvert()
		{
			FileFormat defaultFileFormat = _cmd.fileFormat == kFormatSuffixes[FileFormat::LibSVM] ? FileFormat::Unknown : FileFormat::LibSVM;
			FileFormat fileFormat = get_value(kFormats, _cmd.outputFileFormat, defaultFileFormat);
			Instances instances = create_instances(_cmd.datafile);
			if (fileFormat == FileFormat::Unknown)
			{
				LOG(WARNING) << "Not specified ouput file format";
			}
			else if (fileFormat == instances.schema.fileFormat)
			{
				LOG(WARNING) << "Specified ouput file format is the same as input";
			}
			//else
			{
				string outfile = _cmd.outfile;
				if (outfile.empty())
				{
					string suffix = kFormatSuffixes[fileFormat];
					outfile = GetOutputFileName(_cmd.datafile, suffix, true);
					if (outfile == _cmd.datafile)
					{
						outfile += ".bak";
					}
				}
				write(instances, outfile, fileFormat);
			}
		}

		void SplitDataByLabel(Instances& instances)
		{
			Instances posInstances(instances.schema);
			Instances negInstances(instances.schema);
			for (InstancePtr instance : instances)
			{
				if (instance->IsPositive())
					posInstances.push_back(instance);
				else
					negInstances.push_back(instance);
			}
			{
				string outfile = GetOutputFileName(_cmd.datafile, "pos");
				Pval(outfile);
				write(posInstances, outfile);
			}
			{
				string outfile = GetOutputFileName(_cmd.datafile, "neg");
				Pval(outfile);
				write(negInstances, outfile);
			}
		}
		//��ǰ����cross fold˼· 
		void RunSplitData()
		{
			auto instances = create_instances(_cmd.datafile);
			if (_cmd.commandInput.empty())
			{
				VLOG(0) << "No input assume to split by label";
				SplitDataByLabel(instances);
				return;
			}
			RandomEngine rng = random_engine(_cmd.randSeed);
			if (!_cmd.foldsSequential)
				instances.Randomize(rng);

			ivec segs;
			try
			{
				segs.resize(boost::lexical_cast<int>(_cmd.commandInput), 1);
			}
			catch (...)
			{
				svec segs_ = split(_cmd.commandInput, ':');
				if (segs_.size() <= 1)
				{
					LOG(WARNING) << "Need input like -ci 1:1  -ci 1:3:2 or -ci 5";
					return;
				}
				segs = from(segs_) >> select([](string a) { return INT(a); }) >> to_vector();
			}
			_cmd.numFolds = sum(segs);
			Pval(_cmd.numFolds);
			int partNum = segs.size();
			ivec instanceFoldIndices = CVFoldCreator::CreateFoldIndices(instances, _cmd, rng);
			vector<Instances> parts(partNum);

			ivec maps(_cmd.numFolds);
			int idx = 0;
			for (int i = 0; i < partNum; i++)
			{
				for (int j = 0; j < (int)segs[i]; j++)
				{
					maps[idx++] = i;
				}
			}

			for (int i = 0; i < partNum; i++)
			{
				parts[i].CopySchema(instances.schema);
			}

			for (size_t i = 0; i < instances.size(); i++)
			{
				parts[maps[instanceFoldIndices[i]]].push_back(instances[i]);
			}

			string infile = _cmd.datafile;
			for (int i = 0; i < partNum; i++)
			{
				string suffix = STR(i) + "_" + STR(partNum);
				string outfile = GetOutputFileName(infile, suffix);
				Pval(outfile);
				write(parts[i], outfile);
			}
		}

		void 	RunGenCrossData()
		{
			//������ʲô��ʽ �������ʲô��ʽ ��������ʽ����������libsvm��� 
			FileFormat fileFormat = get_value(kFormats, _cmd.outputFileFormat, FileFormat::LibSVM);
			auto instances = create_instances(_cmd.datafile);
			string outDir = _cmd.outDir.empty() ? "cross-data" : _cmd.outDir;
			try_create_dir(outDir);
			//���� feature.txt �����outDir��  feature.train_0.txt feature.test_0.txt feature.train_1.txt ...
			//������runIdx ͳһ�ۼ� 0,1,2...
			const int randomStep = 10000; //@TODO
			for (int runIdx = 0; runIdx < _cmd.numRuns; runIdx++)
			{
				VLOG(0) << "The " << runIdx << " round";
				RandomEngine rng = random_engine(_cmd.randSeed, runIdx * randomStep);
				if (!_cmd.foldsSequential)
					instances.Randomize(rng);

				ivec instanceFoldIndices = CVFoldCreator::CreateFoldIndices(instances, _cmd, rng);
				for (int foldIdx = 0; foldIdx < _cmd.numFolds; foldIdx++)
				{
					VLOG(0) << "Cross validaion foldIdx " << foldIdx;
					int idx = runIdx * _cmd.numFolds + foldIdx;

					Instances trainData, testData;
					//ֻ��trainProportion < 1 ����Ҫrng
					CVFoldCreator::CreateFolds(instances, _cmd.trainProportion,
						instanceFoldIndices, foldIdx, _cmd.numFolds, trainData, testData,
						random_engine(_cmd.randSeed, runIdx * randomStep));

					string trainSuffix = "feature.train_" + STR(idx);
					string trainFile = outDir + "/" + trainSuffix;
					string testSuffix = "feature.test_" + STR(idx);
					string testFile = outDir + "/" + testSuffix;
					Pval2(trainFile, testFile);

					write(trainData, trainFile, fileFormat);
					write(testData, testFile, fileFormat);
				}
			}
		}

		//�ı��������ı���
		void RunChangeRatio()
		{
			auto instances = create_instances(_cmd.datafile);
			RandomEngine rng = random_engine(_cmd.randSeed);
			if (!_cmd.foldsSequential)
				instances.Randomize(rng);
			svec segs = split(_cmd.commandInput, ':');
			int partNum = (int)segs.size();
			if (partNum != 2)
			{
				LOG(WARNING) << "Need input like -ci 1:2 -ci 1:2 the part num should be 2 not " << partNum;
				return;
			}

			double posPart = DOUBLE(segs[0]);
			double negPart = DOUBLE(segs[1]);

			uint64 posNum = instances.PositiveCount();
			uint64 negNum = instances.Count() - posNum;

			uint64 posAdjustedNum = negNum / negPart * posPart;

			string infile = _cmd.datafile;
			string suffix = replace(_cmd.commandInput, ':', '-');
			string outfile = GetOutputFileName(infile, suffix);
			if (posAdjustedNum == posNum)
			{
				LOG(WARNING) << "Need to do nothing";
			}
			else
			{
				Instances newInstances(instances.schema);
				if (posAdjustedNum > posNum)
				{
					uint64 negAdjustedNum = posNum / posPart * negPart;
					VLOG(0) << "Shrink neg part num to " << negAdjustedNum;
					uint64 negCount = 0;
					for (InstancePtr instance : instances)
					{
						if (instance->IsNegative())
						{
							if (negCount >= negAdjustedNum)
							{
								continue;
							}
							negCount++;
						}
						newInstances.push_back(instance);
					}
				}
				else
				{
					VLOG(0) << "Shrink pos part num to " << posAdjustedNum;
					uint64 posCount = 0;
					for (InstancePtr instance : instances)
					{
						if (instance->IsPositive())
						{
							if (posCount >= posAdjustedNum)
							{
								continue;
							}
							posCount++;
						}
						newInstances.push_back(instance);
					}
				}
				Pval(outfile);
				write(newInstances, outfile);
			}
		}

		void RunWriteTextModel()
		{
			if (!_cmd.modelfileText && !_cmd.modelfileXml && !_cmd.modelfileJson)
			{
				LOG(WARNING) << "Will do nothing, you have to set -mt 1 or -mxml 1 or -mjson 1";
				return;
			}

			Noticer nt("WiteTextModel! with model from " + _cmd.modelFolder);
			//------load predictor
			PredictorPtr predictor;
			if (!_cmd.saveNormalizerText && !_cmd.saveCalibratorText)
			{
				Predictor::loadNormalizerAndCalibrator() = false;
			}
		
			{
				Noticer nt("Loading predictor");
				predictor = PredictorFactory::LoadPredictor(_cmd.modelFolder);
			}

			(*predictor).SetSaveNormalizerText(_cmd.saveNormalizerText)
				.SetSaveCalibratorText(_cmd.saveCalibratorText)
				.SetPath(_cmd.modelFolder);

			if (_cmd.modelfileXml)
			{
				predictor->SaveXml();
			}
			if (_cmd.modelfileJson)
			{
				predictor->SaveJson();
			}
			if (_cmd.modelfileText)
			{
				predictor->SaveText();
			}
		}

		void RunTextModelToBinary()
		{
			Noticer nt("TextModelToBinary! with model from " + _cmd.modelFolder);
			//------load predictor
			PredictorPtr predictor;
			Predictor::loadNormalizerAndCalibrator() = false;
			{
				Noticer nt("Loading predictor");
				predictor = PredictorFactory::CreatePredictorFromTextFormat(_cmd.classifierName, _cmd.modelFolder);
			}
			predictor->Save(_cmd.modelFolder);
		}

		void RunExperiments()
		{
			Pval(omp_get_num_procs());
			if (_cmd.numThreads)
			{
				omp_set_num_threads(_cmd.numThreads);
			}
			else
			{
				omp_set_num_threads(omp_get_num_procs());
			}
			Pval(get_num_threads());
			//��������ģʽ
			string commandStr = erase(boost::to_lower_copy(_cmd.command), "_-");
			Pval(commandStr);
			RunType command = get_value(_commands, commandStr, RunType::UNKNOWN);
			switch (command)
			{
			case RunType::EVAL:
				RunCrossValidation();
				break;
			case  RunType::EVAL_PARAM:
				RunCrossValidation(CrossValidationType::AUC);
				break;
			case RunType::TRAIN:
				RunTrain();
				break;
			case  RunType::TEST:
				RunTest();
				break;
			case RunType::TRAIN_TEST:
				RunTrainTest();
				break;
			case  RunType::FEATURE_SELECTION:
				RunFeatureSelection();
				break;
			case  RunType::CREATE_INSTANCES:
				RunCreateInstances();
				break;
			case RunType::NORMALIZE:
				RunNormalize();
				break;
			case RunType::CALIBRATE:
				RunCalibrate();
				break;
			case RunType::CHECK_DATA:
				RunCheckData();
				break;
			case RunType::FEATURE_STATUS:
				RunFeatureStatus();
				break;
			case RunType::SHOW_FEATURES:
				RunShowFeatures();
				break;
			case RunType::SHOW_INFOS:
				RunShowInfos();
				break;
			case RunType::CONVERT:
				RunConvert();
				break;
			case RunType::SPLIT_DATA:
				RunSplitData();
				break;
			case  RunType::GEN_CROSS_DATA:
				RunGenCrossData();
				break;
			case RunType::CHANGE_RAIO:
				RunChangeRatio();
				break;
			case RunType::WRITE_TEXT_MODEL:
				RunWriteTextModel();
				break;
			case  RunType::TEXT_MODEL_TO_BINARY:
				RunTextModelToBinary();
				break;
			case RunType::HELP:
				PrintCommands();
				break;
			case RunType::UNKNOWN:
			default:
				LOG(WARNING) << commandStr << " is not supported yet ";
				PrintCommands();
				break;
			}
		}

	protected:
	private:
		MeltArguments _cmd;
		map<string, RunType> _commands = {
			{ "help", RunType::HELP },
			{ "cv", RunType::EVAL },
			{ "eval", RunType::EVAL },
			{ "eval_param", RunType::EVAL_PARAM },
			{ "cv2", RunType::EVAL_PARAM },
			{ "cv_param", RunType::EVAL_PARAM },
			{ "auc", RunType::EVAL_PARAM },
			{ "train", RunType::TRAIN },
			{ "tr", RunType::TRAIN },
			{ "test", RunType::TEST },
			{ "te", RunType::TEST },
			{ "traintest", RunType::TRAIN_TEST },
			{ "tt", RunType::TRAIN_TEST },
			{ "featureselection", RunType::FEATURE_SELECTION },
			{ "fs", RunType::FEATURE_SELECTION },
			{ "createinstances", RunType::CREATE_INSTANCES },
			{ "ci", RunType::CREATE_INSTANCES },
			{ "norm", RunType::NORMALIZE },
			{ "calibrate", RunType::CALIBRATE },
			{ "check", RunType::CHECK_DATA },
			{ "featurestatus", RunType::FEATURE_STATUS },
			{ "fss", RunType::FEATURE_STATUS },
			{ "showfeatures", RunType::SHOW_FEATURES },
			{ "sf", RunType::SHOW_FEATURES },
			{ "showinfos", RunType::SHOW_INFOS },
			{ "si", RunType::SHOW_INFOS },
			{ "convert", RunType::CONVERT },
			{ "splitdata", RunType::SPLIT_DATA },
			{ "sd", RunType::SPLIT_DATA },
			{ "gen_crossdata", RunType::GEN_CROSS_DATA },
			{ "gcd", RunType::GEN_CROSS_DATA },
			{ "changeratio", RunType::CHANGE_RAIO },
			{ "cr", RunType::CHANGE_RAIO },
			{ "write_text_model", RunType::WRITE_TEXT_MODEL },
			{ "wtm", RunType::WRITE_TEXT_MODEL },
			{ "binary_model_to_text", RunType::WRITE_TEXT_MODEL },
			{ "bm2t", RunType::WRITE_TEXT_MODEL },
			{ "text_model_to_binary", RunType::TEXT_MODEL_TO_BINARY },
			{ "tm2b", RunType::TEXT_MODEL_TO_BINARY }
		};

	};
} //end of namespace gezi


#endif  //----end of RUN__MELT_H_
