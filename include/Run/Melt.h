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
#include "Testers/testers.h"
#include "Utils/PredictorUtils.h"

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
			HELP_TRAINERS, //Melt����֧�ֵ�trainers��Ϣ��ӡ
			HELP_TRAINER, //��ӡ��ǰ-clָ����trainer��Help��Ϣ(���tainer��ʵ��ShowHelp)
			EVAL, //������֤,Ĭ��ִ�е�command
			EVAL_PARAM, //������֤ ����ֻ���auc��ֵ,��Ҫ���ڼ�ⲻͬ����Ч���Ա�
			TRAIN, //ѵ��
			TEST,  //����
			TRAIN_TEST,  //ѵ��+����
			FEATURE_SELECTION,  //����ѡ��  //����ѡ��ŵ���Χpython�ű� 
			CREATE_INSTANCES,  //��������normal��ʽ������ת��Ϊ���ϵ�,����catogry,text���չ��
			NORMALIZE,  //���й�һ�������������ı�,��normalizer��Ϣ���л���-m��Ӧ��·����
			NORMALIZE_FILE, //�����ı����й�һ��������������ı�
			CALIBRATE,  //����output->[0-1]�Ĺ�һ������calibrator��Ϣ���л���-m��Ӧ��·����
			CHECK_DATA, //������� ��ǰ�ǽ���minmax��һ ��ʾ��������
			FEATURE_STATUS, //��ӡ�����ľ�ֵ������Ϣ��mean var
			SHOW_FEATURES, //��ӡ������
			SHOW_INFOS, //չʾ�������ݵĻ�����Ϣ  ������Ŀ��������Ŀ����������
			CONVERT, //�����������ļ�����Ȼ�����Ҫ��ĸ�ʽ ���� dense -> sparse
			SPLIT_DATA, //���������������з�  ���� 1:1 1:3:2 ͬʱ����ÿ��������������ά�ֺ�ԭʼ����һ��
			GEN_CROSS_DATA, //���������֤���ı������ļ� ����Ա���������ѧϰ���ߵ�ʵ��Ч��
			CHANGE_RAIO, //���������������������������� ���� ԭʼ 1:30 ����Ϊ 1:1
			RANDOMIZE, //���������������������� -num > 0��ֻ���ǰnum������ ������ -ci fr ����  -ci forceRatio ��ô��֤����,Ĭ���ǲ���֤����
			WRITE_TEXT_MODEL, //����binaryģ�ͺ�д���ı���ʽģ����-mt -mxml -mjson ע��ģ���ڲ���normalizer,calibratorĬ�϶��ǲ����Text��ʽ�ģ������Ҫ��� -snt 1, -sct 1
			TEXT_MODEL_TO_BINARY //��ȡ-m ָ��·���µ�model.txt �û�ָ��ģ������-cl Ĭ����LinearSVM �����ı���ʽģ������д��binaryģ�͵�-m·��
		};


		//Ŀǰ���ݵ����ⲿ�ű� ����û������չ�� ��ʱֻ���Ƕ����࣬�����õ�Tester�����˿���չ��
		enum class CrossValidationType
		{
			DEFAULT = 0, //Ĭ��ʹ��melt��������c++��Tester
			USE_SCRIPT, //�����ⲿpython�ű�����instance�ļ�����evaluate��� ��~/tools/evaluate.py ~/tools/evaluate.full.py���Ը������ROC,PR���ߣ�������ֵչʾ�ٻ�
			AUC //������instance�ļ�,ֻ���ڲ�����auc��������evaluate����@TODO,���ڲ���ѡȡ
		};

		MeltArguments& Cmd()
		{
			return _cmd;
		}

		void PrintCommands()
		{
			VLOG(0) << "Supported commands now are below: [commandName or commandShortName] <-> [commandId]";
			print_enum_map(_commands);
			int i = (int)RunType::HELP_TRAINERS; // 0 UNKNOWN, 1 HELP, 2 HELP_TRAINERS
			VLOG(0) << i++ << " HELP_TRAINERS, //Melt����֧�ֵ�trainers��Ϣ��ӡ";
			VLOG(0) << i++ << " HELP_TRAINER, //��ӡ��ǰ-clָ����trainer��Help��Ϣ(���tainer��ʵ��ShowHelp)";
			VLOG(0) << i++ << " EVAL, //������֤,Ĭ��ִ�е�command";
			VLOG(0) << i++ << " EVAL_PARAM, //������֤ ����ֻ���auc��ֵ,��Ҫ���ڼ�ⲻͬ����Ч���Ա�";
			VLOG(0) << i++ << " TRAIN, //ѵ��(-mt -mxml -mjson���ÿ��������Ӧ�ı���ʽģ�ͣ����Ҫ���ڲ���normalizer�����Ӧ�ı���ʽ���� -snt 1,calibrator���� -sct 1)";
			VLOG(0) << i++ << " TEST,  //����";
			VLOG(0) << i++ << " TRAIN_TEST,  //ѵ��+����";
			VLOG(0) << i++ << " FEATURE_SELECTION,  //����ѡ��  //����ѡ��ŵ���Χpython�ű�";
			VLOG(0) << i++ << " CREATE_INSTANCES,  //��������normal��ʽ������ת��Ϊ���ϵ�,����catogry,text���չ��";
			VLOG(0) << i++ << " NORMALIZE,  //���й�һ�������������ı�,��normalizer��Ϣ���л���-m��Ӧ��·����(-mt -mxml -mjson���ÿ��������Ӧ�ı���ʽ ����Ҫ����-snt)";
			VLOG(0) << i++ << " NORMALIZE_FILE, //�����ı����й�һ��������������ı�";
			VLOG(0) << i++ << " CALIBRATE,  //����output->[0-1]�Ĺ�һ������calibrator��Ϣ���л���-m��Ӧ��·����  -mt -mxml -mjson���ÿ��������Ӧ�ı���ʽ ����Ҫ����-sct";
			VLOG(0) << i++ << " CHECK_DATA, //������� ��ǰ�ǽ���minmax��һ ��ʾ��������";
			VLOG(0) << i++ << " FEATURE_STATUS, //��ӡ�����ľ�ֵ������Ϣ��mean var";
			VLOG(0) << i++ << " SHOW_FEATURES, //��ӡ������";
			VLOG(0) << i++ << " SHOW_INFOS, //չʾ�������ݵĻ�����Ϣ  ������Ŀ��������Ŀ����������";
			VLOG(0) << i++ << " CONVERT, //�����������ļ�����Ȼ�����Ҫ��ĸ�ʽ ���� dense -> sparse";
			VLOG(0) << i++ << " SPLIT_DATA, //���������������з�  ���� 1:1 1:3:2 ͬʱ����ÿ��������������ά�ֺ�ԭʼ����һ��";
			VLOG(0) << i++ << " GEN_CROSS_DATA, //���������֤���ı������ļ� ����Ա���������ѧϰ���ߵ�ʵ��Ч��";
			VLOG(0) << i++ << " CHANGE_RAIO //���������������������������� ���� ԭʼ 1:30 ����Ϊ 1:1";
			VLOG(0) << i++ << " RANDOMIZE //���������������������� -num > 0��ֻ���ǰnum������ ������ -ci fr ����  -ci forceRatio ��ô��֤����,Ĭ���ǲ���֤����";
			VLOG(0) << i++ << " WRITE_TEXT_MODEL // ����binaryģ�ͺ�д���ı���ʽģ����-mt -mxml -mjson(ע���ڲ���normalizer�����Ҫ�ı������Ҫ-snt 1,���Ƶ�calibrator�ı���� -sct 1)";
			VLOG(0) << i++ << " TEXT_MODEL_TO_BINARY //��ȡ-m ָ��·���µ�model.txt �û�ָ��ģ������-cl Ĭ����LinearSVM �����ı���ʽģ������д��binaryģ�͵�-m·��";
		}

		void RunCrossValidation(Instances& instances, CrossValidationType cvType)
		{
			//--------------------------- ����ļ�ͷ
			string fullInstFile = _cmd.resultDir + "/" + STR(_cmd.resultIndex) + ".inst.txt";
			ofstream ofs; //��� cvType == CrossValidationType::USE_SCRIPT  ʹ��
			if (cvType == CrossValidationType::USE_SCRIPT || cvType == CrossValidationType::DEFAULT)
			{
				try_create_dir(_cmd.resultDir);
				if (cvType == CrossValidationType::USE_SCRIPT)
				{
					ofs.open(fullInstFile);
					WriteInstFileHeader(ofs);
				}
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
			TesterPtr tester = nullptr;
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
					string instfile = format("{}/{}_{}_{}.inst.txt", _cmd.resultDir, _cmd.resultIndex
						, runIdx, foldIdx);

					Instances trainData, testData;
					//ֻ��trainProportion < 1 ����Ҫrng
					CVFoldCreator::CreateFolds(instances, _cmd.trainProportion,
						instanceFoldIndices, foldIdx, _cmd.numFolds, trainData, testData,
						random_engine(_cmd.randSeed, runIdx * randomStep));

					//------------------------------------Train
					TrainerPtr trainer = TrainerFactory::CreateTrainer(_cmd.classifierName);
					CHECK(trainer != nullptr);
					if (foldIdx == 0)
					{
						VLOG(0) << "Folds " << foldIdx << " are trained with " << trainData.Size() << " instances, and tested on " << testData.Size() << " instances";
						PVAL3(trainData[0]->name, trainData.PositiveCount(), trainData.NegativeCount());
						PVAL3(testData[0]->name, testData.PositiveCount(), testData.NegativeCount());
					}

					trainer->SetNormalizeCopy();
					trainer->Train(trainData);
					PredictorPtr predictor = trainer->CreatePredictor();
					predictor->SetNormalizeCopy();

					if (cvType == CrossValidationType::DEFAULT)
					{
						if (tester == nullptr)
						{
#pragma  omp critical
							{
								tester = PredictorUtils::GetTester(predictor);
								tester->isCrossValidationMode = true;
							}
						}
						else
						{
							tester->writeTSVHeader = false;
						}
						tester->Test(testData, predictor, fullInstFile);
					}
					else if (cvType == CrossValidationType::USE_SCRIPT)
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
				tester->Finalize();
			}
			else if (cvType == CrossValidationType::USE_SCRIPT)
			{
				string command = _cmd.evaluate + fullInstFile;
#pragma omp critical
				{
					EXECUTE(command);
				}
			}
			else if (cvType == CrossValidationType::AUC)
			{
				double auc = evaluator->Finish();
				cout << auc << "\t" << "trainerParam: " << trainerParam << endl;
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
			if (cvType == CrossValidationType::DEFAULT && !_cmd.evaluate.empty())
			{
				cvType = CrossValidationType::USE_SCRIPT;
			}
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
				CHECK(!std::isnan(output));
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

		PredictorPtr Train(Instances& instances, bool normalizeCopy = false)
		{
			Pval(_cmd.classifierName);
			auto trainer = TrainerFactory::CreateTrainer(_cmd.classifierName);
			CHECK(trainer != nullptr);
			trainer->SetNormalizeCopy(normalizeCopy);
			trainer->Train(instances);
			auto predictor = trainer->CreatePredictor();
			predictor->SetParam(trainer->GetParam());
			return predictor;
		}

		void PrintTrainerInfo()
		{
			Pval(_cmd.classifierName);
			auto trainer = TrainerFactory::CreateTrainer(_cmd.classifierName);
			if (trainer == nullptr)
			{
				LOG(WARNING) << _cmd.classifierName << " has not been supported yet";
				return;
			}
			trainer->ShowHelp();
		}

		void RunTrain()
		{
			PredictorPtr predictor;
			Instances instances;
			{
				Noticer nt("Train!");
				instances = create_instances(_cmd.datafile);
				CHECK_GT(instances.Count(), 0) << "Read 0 test instances, aborting experiment";
				predictor = Train(instances, _cmd.selfTest);
			}
			if (_cmd.selfTest)
			{
				Noticer nt("Test itself!");
				try_create_dir(_cmd.resultDir);
				string instFile = _cmd.resultDir + "/" + STR(_cmd.resultIndex) + ".inst.txt";
				//auto testInstances = create_instances(_cmd.datafile);
				auto& testInstances = instances; //train�Ĺ�����û�иı�instance�� ����normalize copy
				CHECK_GT(testInstances.Count(), 0) << "Read 0 test instances, aborting experiment";
				if (!_cmd.evaluate.empty())
				{
					Test(testInstances, predictor, instFile);
					string command = _cmd.evaluate + instFile;
					EXECUTE(command);
				}
				else
				{
					auto tester = PredictorUtils::GetTester(predictor);
					tester->Test(testInstances, predictor, instFile);
				}
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
				CHECK(predictor != nullptr);
			}
			//------test
			try_create_dir(_cmd.resultDir);
			string instFile = _cmd.resultFile.empty() ? format("{}/{}.inst.txt", _cmd.resultDir, _cmd.resultIndex) : _cmd.resultFile;

			//@TODO hack for text input format //Not tested correctness yet
			InstanceParser::TextFormatParsedTime(); //++ pared text from time������ʾ��Ҫʹ�ôʱ�����
			string testDatafile = _cmd.testDatafile.empty() ? _cmd.datafile : _cmd.testDatafile;
			auto testInstances = create_instances(testDatafile);
			CHECK_GT(testInstances.Count(), 0) << "Read 0 test instances, aborting experiment";
			if (!_cmd.evaluate.empty())
			{
				Test(testInstances, predictor, instFile);
				string command = _cmd.evaluate + instFile;
				EXECUTE(command);
			}
			else
			{
				auto tester = PredictorUtils::GetTester(predictor);
				tester->Test(testInstances, predictor, instFile);
			}
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
				CHECK(predictor != nullptr);
			}
			{
				Noticer nt("Test!");
				try_create_dir(_cmd.resultDir);
				string instFile = _cmd.resultFile.empty() ? format("{}/{}.inst.txt", _cmd.resultDir, _cmd.resultIndex) : _cmd.resultFile;

				auto testInstances = create_instances(_cmd.testDatafile);
				CHECK_GT(testInstances.Count(), 0) << "Read 0 test instances, aborting experiment";
				CHECK_EQ(instances.schema == testInstances.schema, 1);
				if (!_cmd.evaluate.empty())
				{
					Test(testInstances, predictor, instFile);
					string command = _cmd.evaluate + instFile;
					EXECUTE(command);
				}
				else
				{
					auto tester = PredictorUtils::GetTester(predictor);
					tester->Test(testInstances, predictor, instFile);
				}
			}
			if (_cmd.modelfile)
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
			fmt::print("Num features: {}\n", instances.NumFeatures());
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
			//string suffix = normalizer->Name() + ".normed";
			string suffix = "normed";
			string outfile = _cmd.outfile.empty() ? GetOutputFileName(infile, suffix) : _cmd.outfile;
			Pval(outfile);

			Instances instances = create_instances(_cmd.datafile);

			normalizer->RunNormalize(instances);

			if (_cmd.saveOutputFile)
			{
				//FileFormat fileFormat = get_value(kFormats, _cmd.outputFileFormat, FileFormat::Unknown);
				FileFormat fileFormat = kFormats[_cmd.outputFileFormat];
				write(instances, outfile, fileFormat);
			}

			try_create_dir(_cmd.modelFolder);
			SAVE_SHARED_PTR_ALL(normalizer);
		}

		void RunNormalizeFile()
		{
			Noticer nt("Convert to normalized file!");
			NormalizerPtr normalizer = NormalizerFactory::CreateNormalizer(_cmd.normalizerName);
			CHECK(normalizer != nullptr);
			Pval(normalizer->Name());

			string infile = _cmd.datafile;
			//string suffix = normalizer->Name() + ".normed";
			string suffix = "normed";
			string outfile = _cmd.outfile.empty() ? GetOutputFileName(infile, suffix) : _cmd.outfile;
			Pval(outfile);

			Instances instances = create_instances(_cmd.datafile);

			normalizer->RunNormalize(instances);

			//FileFormat fileFormat = get_value(kFormats, _cmd.outputFileFormat, FileFormat::Unknown);
			FileFormat fileFormat = kFormats[_cmd.outputFileFormat];
			write(instances, outfile, fileFormat);
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
			Noticer nt("CheckData!(need GLOG_v=4 or -vl 4), this command is derecated try use -c fss -vl 1");
			Instances instances = create_instances(_cmd.datafile);
			NormalizerPtr normalizer = make_shared<MinMaxNormalizer>();
			normalizer->Prepare(instances);
		}

		void RunShowInfos()
		{
			auto instances = create_instances(_cmd.datafile, true); //�ڲ���ӡ��Ϣ
		}

		void RunFeatureStatus()
		{
			Noticer nt("FeatureStatus! You may try to use -vl 1 to print warning of possible no use features");
			string infile = _cmd.datafile;
			string suffix = "featurestatus";
			string outfile = _cmd.outfile.empty() ? GetOutputFileName(infile, suffix)
				: _cmd.outfile;
			string outfile2 = _cmd.outfile.empty() ? GetOutputFileName(infile, format("{}.csv", suffix), true)
				: format("{}.csv", _cmd.outfile);
			Instances instances = create_instances(_cmd.datafile);
			FeatureStatus::GenMeanVarInfo(instances, outfile, outfile2, _cmd.featureName);
		}

		//�����ļ�ת�������
		void RunConvert()
		{
			FileFormat defaultFileFormat = _cmd.inputFileFormat == kFormatNames[FileFormat::LibSVM] ? FileFormat::Unknown : FileFormat::LibSVM;
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
					string suffix = kFormatNames[fileFormat];
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
			//FileFormat fileFormat = get_value(kFormats, _cmd.outputFileFormat, FileFormat::Unknown);
			FileFormat fileFormat = kFormats[_cmd.outputFileFormat];
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
				write(negInstances, outfile, fileFormat);
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
			//FileFormat fileFormat = get_value(kFormats, _cmd.outputFileFormat, FileFormat::Unknown);
			FileFormat fileFormat = kFormats[_cmd.outputFileFormat];
			for (int i = 0; i < partNum; i++)
			{
				string suffix = STR(i) + "_" + STR(partNum);
				string outfile = GetOutputFileName(infile, suffix);
				{
					string suffix = kFormatSuffixes[fileFormat];
					if (suffix != "txt")
					{
						outfile = GetOutputFileName(outfile, suffix, true);
					}
				}
				Pval(outfile);
				write(parts[i], outfile, fileFormat);
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

			size_t posNum = instances.PositiveCount();
			size_t negNum = instances.Count() - posNum;

			size_t posAdjustedNum = negNum / negPart * posPart;

			string infile = _cmd.datafile;
			string suffix = replace(_cmd.commandInput, ':', '-');
			string outfile = GetOutputFileName(infile, suffix);
			FileFormat fileFormat = get_value(kFormats, _cmd.outputFileFormat, FileFormat::Unknown);
			if (posAdjustedNum == posNum)
			{
				LOG(WARNING) << "Need to do nothing";
			}
			else
			{
				Instances newInstances(instances.schema);
				if (posAdjustedNum > posNum)
				{
					size_t negAdjustedNum = posNum / posPart * negPart;
					VLOG(0) << "Shrink neg part num to " << negAdjustedNum;
					size_t negCount = 0;
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
					size_t posCount = 0;
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
				write(newInstances, outfile, fileFormat);
			}
		}

		void RunRandomize()
		{
			auto instances = create_instances(_cmd.datafile);
			RandomEngine rng = random_engine(_cmd.randSeed);
			if (!_cmd.foldsSequential)
			{
				instances.Randomize(rng);
			}
			FileFormat fileFormat = get_value(kFormats, _cmd.outputFileFormat, FileFormat::Unknown);
			string suffix = "rand";
			if (_cmd.num > 0)
			{
				suffix = format("{}.{}", suffix, _cmd.num);
			}
			string outfile = _cmd.outfile.empty() ? GetOutputFileName(_cmd.datafile, suffix) : _cmd.outfile;
			Pval(outfile);

			if (_cmd.num > 0 && _cmd.num < (int64)instances.Count())
			{
				Instances newInstances(instances.schema);
				if (_cmd.commandInput == "fr" || _cmd.commandInput == "forceRatio")
				{ //��֤��������
					double posRatio = instances.PositiveCount() / (double)instances.Count();
					size_t posCount = (size_t)(_cmd.num * posRatio + 0.5);
					size_t negCount = _cmd.num - posCount;
					size_t numPoses = 0, numNegs = 0;
					for (size_t i = 0; i < instances.Count(); i++)
					{
						if (instances[i]->IsPositive())
						{
							if (numPoses < posCount)
							{
								numPoses++;
								newInstances.push_back(instances[i]);
								if (numPoses + numNegs == (size_t)_cmd.num)
								{
									break;
								}
							}
						}
						else
						{
							if (numNegs < negCount)
							{
								numNegs++;
								newInstances.push_back(instances[i]);
								if (numPoses + numNegs == (size_t)_cmd.num)
								{
									break;
								}
							}
						}
					}
				}
				else
				{ //��ȫ�������
					for (int i = 0; i < _cmd.num; i++)
					{
						newInstances.push_back(instances[i]);
					}
				}
				write(newInstances, outfile, fileFormat);
			}
			else
			{
				write(instances, outfile, fileFormat);
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
			{ //@TODO openmp�����߳���Ŀ��΢��� �����������������12�� ����12 ���� 11��13 �ȶ���12��ܶࡣ��
				int numProcs = omp_get_num_procs();
				numProcs = std::max(1, numProcs - 2);
				omp_set_num_threads(numProcs);
			}
			Pval(get_num_threads());
			//��������ģʽ
			string commandStr = arg(_cmd.command);
			Pval(commandStr);
			//RunType command = get_value(_commands, commandStr, RunType::UNKNOWN);
			RunType command = _commands[commandStr];
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
			case RunType::NORMALIZE_FILE:
				RunNormalizeFile();
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
			case RunType::RANDOMIZE:
				RunRandomize();
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
			case RunType::HELP_TRAINERS:
				TrainerFactory::PrintTrainersInfo();
				break;
			case RunType::HELP_TRAINER:
				PrintTrainerInfo();
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
			{ "helptrainers", RunType::HELP_TRAINERS },
			{ "hts", RunType::HELP_TRAINERS },
			{ "helptrainer", RunType::HELP_TRAINER },
			{ "ht", RunType::HELP_TRAINER },
			{ "cv", RunType::EVAL },
			{ "eval", RunType::EVAL },
			{ "evalParam", RunType::EVAL_PARAM },
			{ "cv2", RunType::EVAL_PARAM },
			{ "cvParam", RunType::EVAL_PARAM },
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
			{ "normfile", RunType::NORMALIZE_FILE },
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
			{ "gencrossdata", RunType::GEN_CROSS_DATA },
			{ "gcd", RunType::GEN_CROSS_DATA },
			{ "changeratio", RunType::CHANGE_RAIO },
			{ "cr", RunType::CHANGE_RAIO },
			{ "randomize", RunType::RANDOMIZE },
			{ "rand", RunType::RANDOMIZE },
			{ "writetextmodel", RunType::WRITE_TEXT_MODEL },
			{ "wtm", RunType::WRITE_TEXT_MODEL },
			{ "binarymodeltotext", RunType::WRITE_TEXT_MODEL },
			{ "bm2t", RunType::WRITE_TEXT_MODEL },
			{ "textmodeltobinary", RunType::TEXT_MODEL_TO_BINARY },
			{ "tm2b", RunType::TEXT_MODEL_TO_BINARY }
		};

	};
} //end of namespace gezi


#endif  //----end of RUN__MELT_H_
