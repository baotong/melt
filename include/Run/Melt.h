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
 *  @TODO ����shrink  ȥ���ض���pos ���� �ض��� neg ʹ�� ���������ﵽԤ����� 1:1 1:2 ��������� Ȼ����������index ˳�򼴿�
 *  ��ʱ ��֧��missing feature, class feature, text feature
 * ����ֻʵ��
 * 1. ������� done
 * 2. Instances ���ݽṹ ϡ�� Dense �Զ�ת��  Done
 * 3. Normalization ��ǰʵ���� MinMax @TODO Gussian and Bin
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

#include "common_util.h"
#include "Run/MeltArguments.h"
#include "Prediction/Instances/InstanceParser.h"
#include "Run/CVFoldCreator.h"
#include "Prediction/Normalization/MinMaxNormalizer.h"
#include "Prediction/Normalization/NormalizerFactory.h"
#include "Utils/FeatureStatus.h"
#include "Prediction/Instances/instances_util.h"
#include "MLCore/TrainerFactory.h"

#include "MLCore/Predictor.h"

#include "Prediction/Instances/instances_util.h"
#include "Predictors/PredictorFactory.h"

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
			EVAL, //������֤
			TRAIN, //ѵ��
			TEST,  //����
			TRAIN_TEST,  //ѵ��+����
			FEATURE_SELECTION,  //����ѡ��  //����ѡ��ŵ���Χpython�ű� 
			CREATE_INSTANCES,  //��������normal��ʽ������ת��Ϊ���ϵ�,����catogry,text���չ��
			NORMALIZE,  //���й�һ�������������ı�
			CHECK_DATA, //������� ��ǰ�ǽ���minmax��һ ��ʾ��������
			FEATURE_STATUS, //��ӡ�����ľ�ֵ������Ϣ��mean var
			SHOW_FEATURES, //��ӡ������
			SHOW_INFOS, //չʾ�������ݵĻ�����Ϣ  ������Ŀ��������Ŀ����������
			CONVERT, //�����������ļ�����Ȼ�����Ҫ��ĸ�ʽ ���� dense -> sparse
			SPLIT_DATA, //���������������з�  ���� 1:1 1:3:2 ͬʱ����ÿ��������������ά�ֺ�ԭʼ����һ��
			CHANGE_RAIO //���������������������������� ���� ԭʼ 1:30 ����Ϊ 1:1
		};

		MeltArguments& Cmd()
		{
			return _cmd;
		}

		void PrintCommands()
		{
			LOG(INFO) << "Supported commands now are below:";
			for (auto item : _commands)
			{
				LOG(INFO) << setiosflags(ios::left) << setfill(' ') << setw(40)
					<< item.first << " " << (int)item.second;
			}
		}

		static string GetOutputFileName(string infile, string suffix)
		{
			return endswith(infile, ".txt") ? boost::replace_last_copy(infile, ".txt", "." + suffix + ".txt") : infile + "." + suffix;
		}

		void RunCrossValidation(Instances& instances)
		{
			//--------------------------- ����ļ�ͷ
			try_create_dir(_cmd.resultDir);
			string instFile = _cmd.resultDir + "/" + STR(_cmd.resultIndex) + ".inst.txt";
			ofstream ofs(instFile);
			WriteInstFileHeader(ofs);

			if (_cmd.preNormalize)
			{
				NormalizerPtr normalizer = NormalizerFactory::CreateNormalizer(_cmd.normalizerName);
				CHECK_NE(normalizer.get(), NULL);
				Pval(normalizer->Name());
				normalizer->RunNormalize(instances);
			}
			const int randomStep = 10000;
			//const int randomStep = 1;
			for (size_t runIdx = 0; runIdx < _cmd.numRuns; runIdx++)
			{
				LOG(INFO) << "The " << runIdx << " round";
				RandomEngine rng = random_engine(_cmd.randSeed, runIdx * randomStep);
				if (!_cmd.foldsSequential)
					instances.Randomize(rng);

				ivec instanceFoldIndices = CVFoldCreator::CreateFoldIndices(instances, _cmd, rng);
#pragma omp parallel for 
				for (size_t foldIdx = 0; foldIdx < _cmd.numFolds; foldIdx++)
				{
					string instfile = (format("%s/%d_%d_%d.inst.txt") % _cmd.resultDir % _cmd.resultIndex
						% runIdx % foldIdx).str();

					Instances trainData, testData;
					//ֻ��trainProportion < 1 ����Ҫrng
					CVFoldCreator::CreateFolds(instances, _cmd.trainProportion,
						instanceFoldIndices, foldIdx, _cmd.numFolds, trainData, testData,
						random_engine(_cmd.randSeed, runIdx * randomStep));

					if (foldIdx == 0)
					{
						VLOG(0) << "Folds " << foldIdx << " are trained with " << trainData.Size() << " instances, and tested on " << testData.Size() << " instances";
						Pval3(trainData[0]->name, trainData.PositiveCount(), trainData.NegativeCount());
						Pval3(testData[0]->name, testData.PositiveCount(), testData.NegativeCount());
					}

					//------------------------------------Train
					TrainerPtr trainer = TrainerFactory::CreateTrainer(_cmd.classifierName);
					trainer->SetNormalizeCopy();
					trainer->Train(trainData);
					PredictorPtr predictor = trainer->CreatePredictor();
					predictor->SetNormalizeCopy();

					//@TODO ÿ��test ���һ��inst �ļ�Ҳ Ȼ��ÿ������һ�����
					LOG(INFO) << "-------------------------------------Testing";
					Test(testData, predictor, instfile, ofs);
					string command = _cmd.evaluate + instfile;
					{
						EXECUTE(command);
					}
				}
			}
			string command = _cmd.evaluate + instFile;
#pragma omp critical
			{
				EXECUTE(command);
			}
		}

		void RunCrossValidation()
		{
			Noticer nt((format("%d fold cross-validation") % _cmd.numFolds).str());
			//----------------------------check if command ok
			CHECK_GE(_cmd.numFolds, 2) << "The number of folds must be at least 2 for cross validation";
			//-----------------------------parse input
			Instances instances = create_instances(_cmd.datafile);
			CHECK_GT(instances.Count(), 0) << "Read 0 instances, aborting experiment";
			instances.PrintSummary();
			//------------------------------run
			RunCrossValidation(instances);
		}

		void WriteInstFileHeader(ofstream& ofs)
		{
			ofs << "Instance\tTrue\tAssigned\tOutput\tProbability" << endl;
		}

		//@TODO ������һ��Ԥ�����
		void Test(Instances& instances, PredictorPtr predictor,
			string outfile, ofstream& ofs)
		{
			Test(instances, predictor, outfile);
#pragma omp critical
			{
				Test(instances, predictor, ofs);
			}
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
			for (InstancePtr instance : instances)
			{
				double output;
				double probability = predictor->Predict(instance, output);
				string name = instance->name.empty() ? STR(idx) : instance->name;
				int assigned = output > 0 ? 1 : 0;
				ofs << name << "\t" << instance->label << "\t"
					<< assigned << "\t" << output << "\t"
					<< probability << endl;
				idx++;
			}
		}

		PredictorPtr Train(Instances& instances)
		{
			Pval(_cmd.classifierName);
			auto trainer = TrainerFactory::CreateTrainer(_cmd.classifierName);
			if (trainer == nullptr)
			{
				return;
			}
			trainer->Train(instances);
			auto predictor = trainer->CreatePredictor();
			return predictor;
		}

		void RunTrain()
		{
			PredictorPtr predictor;
			{
				Noticer nt("Train!");
				auto instances = create_instances(_cmd.datafile);
				CHECK_GT(instances.Count(), 0) << "Read 0 test instances, aborting experiment";
				predictor = Train(instances);
			}
			{
				Noticer nt("Test itself!");
				try_create_dir(_cmd.resultDir);
				string instFile = _cmd.resultDir + "/" + STR(_cmd.resultIndex) + ".inst.txt";
				auto testInstances = create_instances(_cmd.datafile);
				CHECK_GT(testInstances.Count(), 0) << "Read 0 test instances, aborting experiment";
				Test(testInstances, predictor, instFile);
				string command = _cmd.evaluate + instFile;
				EXECUTE(command);
			}
			{
				Noticer nt("Write train result!");
				predictor->Save(_cmd.resultDir + "/" + _cmd.modelFolder);
				if (_cmd.modelfileText)
				{
					predictor->SaveText();
				}
			}
		}

		void RunTest()
		{
			Noticer nt("Test!");
			//------load predictor
			auto predictor = PredictorFactory::LoadPredictor(_cmd.resultDir + "/" + _cmd.modelFolder);
			//------test
			try_create_dir(_cmd.resultDir);
			string instFile = _cmd.resultDir + "/" + STR(_cmd.resultIndex) + ".inst.txt";

			//hack for text input format //Not tested correctness yet
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
				predictor->Save(_cmd.resultDir + "/" + _cmd.modelFolder);
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
			int num = 0;
			for (string feature : instances.FeatureNames())
			{
				std::cout << num++ << "\t" << feature << endl;
			}
		}

		//ֻ��Ϊ�����normalize������� �鿴normalize��Ч��
		void RunNormalizeInstances()
		{
			Noticer nt("NormalizeInstances!");
			string infile = _cmd.datafile;
			string suffix = "normed";
			string outfile = _cmd.outfile.empty() ? GetOutputFileName(infile, suffix) : _cmd.outfile;
			Pval(outfile);
			string normalizerFile = _cmd.normalizerfile.empty() ?
				(endswith(infile, ".txt") ?
				boost::replace_last_copy(infile, ".txt", ".normalizer.txt") : infile + ".normalizer") : _cmd.normalizerfile;
			Pval(normalizerFile);

			Instances instances = create_instances(_cmd.datafile);
			NormalizerPtr normalizer = NormalizerFactory::CreateNormalizer(_cmd.normalizerName);
			CHECK_NE(normalizer.get(), NULL);
			Pval(normalizer->Name());
			normalizer->RunNormalize(instances);
			normalizer->SaveText(normalizerFile);
			FileFormat fileFormat = get_value(_formats, _cmd.outputFileFormat, FileFormat::Unknown);
			write(instances, outfile, fileFormat);
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
			FileFormat fileFormat = get_value(_formats, _cmd.outputFileFormat, FileFormat::Unknown);
			Instances instances = create_instances(_cmd.datafile);
			if (fileFormat == FileFormat::Unknown)
			{
				LOG(WARNING) << "Not specified ouput file format, do nothing";
			}
			else if (fileFormat == instances.schema.fileFormat)
			{
				LOG(WARNING) << "Specified ouput file format is the same as input , do nothing";
			}
			else
			{
				write(instances, _cmd.outfile, fileFormat);
			}
		}

		//��ǰ����cross fold˼· 
		void RunSplitData()
		{
			auto instances = create_instances(_cmd.datafile);
			RandomEngine rng = random_engine(_cmd.randSeed);
			if (!_cmd.foldsSequential)
				instances.Randomize(rng);
			svec segs_ = split(_cmd.commandInput, ':');
			int partNum = (int)segs_.size();
			if (partNum <= 1)
			{
				LOG(WARNING) << "Need input like -ci 1:1  -ci 1:3:2";
				return;
			}
			ivec segs = from(segs_) >> select([](string a) { return INT(a); }) >> to_vector();
			_cmd.numFolds = sum(segs);
			Pval(_cmd.numFolds);
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
			string suffix = _cmd.commandInput;
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
					LOG(INFO) << "Shrink neg part num to " << negAdjustedNum;
					int negCount = 0;
					for (InstancePtr instance : instances)
					{
						if (negCount >= negAdjustedNum)
						{
							continue;
						}
						newInstances.push_back(instance);
						if (instance->IsNegative())
						{
							negCount++;
						}
					}
				}
				else
				{
					LOG(INFO) << "Shrink pos part num to " << posAdjustedNum;
					int posCount = 0;
					for (InstancePtr instance : instances)
					{
						if (posCount >= posAdjustedNum)
						{
							continue;
						}
						newInstances.push_back(instance);
						if (instance->IsPositive())
						{
							posCount++;
						}
					}
				}
				Pval(outfile);
				write(newInstances, outfile);
			}
		}

		void RunExperiments()
		{
			PVAL(omp_get_num_procs());
			if (_cmd.numThreads)
			{
				omp_set_num_threads(_cmd.numThreads);
			}
			//��������ģʽ
			string commandStr = erase(boost::to_lower_copy(_cmd.command), "_-");
			Pval(commandStr);
			RunType command = get_value(_commands, commandStr, RunType::UNKNOWN);
			switch (command)
			{
			case RunType::EVAL:
				RunCrossValidation();
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
				RunNormalizeInstances();
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
			case RunType::CHANGE_RAIO:
				RunChangeRatio();
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
			{ "cv", RunType::EVAL },
			{ "eval", RunType::EVAL },
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
			{ "changeratio", RunType::CHANGE_RAIO },
			{ "cr", RunType::CHANGE_RAIO }
		};

		map<string, FileFormat> _formats = {
			{ "unknown", FileFormat::Unknown },
			{ "dense", FileFormat::Dense },
			{ "sparse", FileFormat::Sparse },
			{ "text", FileFormat::Text },
			{ "libsvm", FileFormat::LibSVM },
			{ "arff", FileFormat::Arff }
		};
	};
} //end of namespace gezi


#endif  //----end of RUN__MELT_H_
