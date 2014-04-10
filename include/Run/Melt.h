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

namespace gezi {
	class Melt
	{
	public:
		Melt()
		{
			ParseArguments();
		}
		~Melt()
		{
		}
		void ParseArguments();

		enum class RunType
		{
			UNKNOWN = 0,
			EVAL,
			TRAIN,
			TEST,
			TRAIN_TEST,
			FEATURE_SELECTION,
			CREATE_INSTANCES,
			NORMALIZE,
			CHECK_DATA,
			FEATURE_STATUS,
			SHOW_FEATURES
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

		void RunCrossValidation(Instances& instances)
		{
			////----------------------------��ͳһ��һ��, ��ı�instances �߼��������в�ͬ
			////����TLC ������ 5 fold��֤ ��ôֻ��4�� ���� ��norm
			//TrainerPtr trainer = TrainerFactory::CreateTrainer(_cmd.classifierName);
			//NormalizerPtr normalizer = trainer->GetNormalizer();
			//if (normalizer != nullptr)
			//{
			//	normalizer->RunNormalize(instances);
			//}

			//--------------------------- ����ļ�ͷ
			try_create_dir(_cmd.resultDir);
			string instFile = _cmd.resultDir + "/" + STR(_cmd.resultIndex) + ".inst.txt";
			ofstream ofs(instFile);
			WriteInstFileHeader(ofs);

			const int randomStep = 10000;
			for (size_t runIdx = 0; runIdx < _cmd.numRuns; runIdx++)
			{
				LOG(INFO) << "The " << runIdx << " round";
				RandomEngine rng = random_engine(_cmd.randSeed, runIdx * randomStep);
				if (!_cmd.foldsSequential)
					instances.Randomize(rng);
				ivec instanceFoldIndices = CVFoldCreator::CreateFoldIndices(instances, _cmd, rng);
//#pragma omp parallel for 
				for (size_t foldIdx = 0; foldIdx < _cmd.numFolds; foldIdx++)
				{
					string instfile = (format("%s/%d_%d_%d.inst.txt") % _cmd.resultDir % _cmd.resultIndex
						% runIdx % foldIdx).str();
					Instances trainData, testData;
					//ֻ��trainProportion < 1 ����Ҫrng
					CVFoldCreator::CreateFolds(instances, _cmd.trainProportion,
						instanceFoldIndices, foldIdx, _cmd.numFolds, trainData, testData,
						random_engine(_cmd.randSeed, runIdx * randomStep));
					LOG(INFO) << "Folds " << foldIdx << " are trained with " << trainData.Size() << " instances, and tested on " << testData.Size() << " instances";
#ifndef NDEBUG
					PVAL3(trainData[0]->name, trainData.PositiveCount(), trainData.NegativeCount());
					PVAL3(testData[0]->name, testData.PositiveCount(), testData.NegativeCount());
#endif // !NDEBUG
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
						system(command.c_str());
					}
				}
			}
			string command = _cmd.evaluate + instFile;
			//#pragma omp critical
			{
				system(command.c_str());
			}
		}

		void RunCrossValidation()
		{
			Noticer nt((format("%d fold cross-validation") % _cmd.numFolds).str());
			//----------------------------check if command ok
			CHECK_GE(_cmd.numFolds, 2) << "The number of folds must be at least 2 for cross validation";
			if (!_cmd.modelfile.empty() || !_cmd.modelfileCode.empty() || !_cmd.modelfileText.empty())
			{
				LOG(FATAL) << "You cannot specify a model file to output when running cross-validation";
			}
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
			//#pragma omp critical
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
			Noticer nt("Train!");
			auto instances = create_instances(_cmd.datafile);
			CHECK_GT(instances.Count(), 0) << "Read 0 test instances, aborting experiment";
			auto predictor = Train(instances);

			try_create_dir(_cmd.resultDir);
			string instFile = _cmd.resultDir + "/" + STR(_cmd.resultIndex) + ".inst.txt";

			LOG(INFO) << "-------------------------------------Testing";
			auto testInstances = create_instances(_cmd.datafile);
			CHECK_GT(testInstances.Count(), 0) << "Read 0 test instances, aborting experiment";
			Test(testInstances, predictor, instFile);
			string command = _cmd.evaluate + instFile;
			system(command.c_str());
			//save predictor
		}

		void RunTest()
		{
			Noticer nt("Test!");
			//------load predictor
			//------test
		}

		void RunTrainTest()
		{
			Noticer nt("TrainTest!");
			auto instances = create_instances(_cmd.datafile);
			CHECK_GT(instances.Count(), 0) << "Read 0 train instances, aborting experiment";
			auto predictor = Train(instances);

			try_create_dir(_cmd.resultDir);
			string instFile = _cmd.resultDir + "/" + STR(_cmd.resultIndex) + ".inst.txt";

			LOG(INFO) << "-------------------------------------Testing";
			auto testInstances = create_instances(_cmd.testDatafile);
			CHECK_GT(testInstances.Count(), 0) << "Read 0 test instances, aborting experiment";
			CHECK_EQ(instances.schema == testInstances.schema, 1);
			Test(testInstances, predictor, instFile);
			string command = _cmd.evaluate + instFile;
			system(command.c_str());
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

		void RunNormalizeInstances()
		{
			Noticer nt("NormalizeInstances!");
			string infile = _cmd.datafile;
			string outfile = endswith(infile, ".txt") ? boost::replace_last_copy(infile, ".txt", ".normed.txt") : infile + ".normed";
			Pval(outfile);
			string normalizerFile = _cmd.normalizerfile.empty() ?
				(endswith(infile, ".txt") ?
				boost::replace_last_copy(infile, ".txt", ".normalizer.txt") : infile + ".normalizer") : _cmd.normalizerfile;

			Instances instances = create_instances(_cmd.datafile);
			NormalizerPtr normalizer = NormalizerFactory::CreateNormalizer(_cmd.normalizerName);
			CHECK_NE(normalizer.get(), NULL);
			Pval(normalizer->Name());
			normalizer->RunNormalize(instances);
			normalizer->Save(normalizerFile);
			//@TODO instances_util.h ���Instancesд��
		}

		void RunCheckData()
		{
			Noticer nt("CheckData!(need GLOG_v=4 ./melt)");
			Instances instances = create_instances(_cmd.datafile);
			NormalizerPtr normalizer = make_shared<MinMaxNormalizer>();
			normalizer->Prepare(instances);
		}

		void RunFeatureStatus()
		{
			Noticer nt("FeatureStatus!");
			string infile = _cmd.datafile;
			string outfile = _cmd.outfile.empty() ? (endswith(infile, ".txt") ? boost::replace_last_copy(infile, ".txt", ".featurestatus.txt") : infile + ".featuestatus")
				: _cmd.outfile;
			Instances instances = create_instances(_cmd.datafile);
			FeatureStatus::GenMeanVarInfo(instances, outfile, _cmd.featureName);
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
			{ "test", RunType::TEST },
			{ "traintest", RunType::TRAIN_TEST },
			{ "tt", RunType::TRAIN_TEST },
			{ "featureselection", RunType::FEATURE_SELECTION },
			{ "fs", RunType::FEATURE_SELECTION },
			{ "createinstances", RunType::CREATE_INSTANCES },
			{ "norm", RunType::NORMALIZE },
			{ "check", RunType::CHECK_DATA },
			{ "featurestatus", RunType::FEATURE_STATUS },
			{ "fss", RunType::FEATURE_STATUS },
			{ "showfeatures", RunType::SHOW_FEATURES },
			{ "sf", RunType::SHOW_FEATURES }
		};
	};
} //end of namespace gezi


#endif  //----end of RUN__MELT_H_
