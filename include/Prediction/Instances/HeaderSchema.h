/**
 *  ==============================================================================
 *
 *          \file   Prediction/Instances/HeaderSchema.h
 *
 *        \author   chenghuige
 *
 *          \date   2014-03-26 13:29:04.257629
 *
 *  \Description:
 *  ==============================================================================
 */

#ifndef PREDICTION__INSTANCES__HEADER_SCHEMA_H_
#define PREDICTION__INSTANCES__HEADER_SCHEMA_H_

#include "common_util.h"
#include "FeatureNamesVector.h"
namespace gezi
{
	//Ĭ��֧�ֵĸ�ʽ��Ӧ�� ���� ��� Instances ���ı���ʽ
	//��Ӧ����֧�ֵ�ģʽ����text, libsvm ��������������ȷ��
	enum class FileFormat
	{
		Unknown = 0,
		Dense,
		Sparse,  // 1230 1:3
		SparseNoLength, // 1:3
		Text,
		LibSVM,
		Arff,
		VW, 
		MallocRank, //��Ҫ��֧��malloc ranking���ݸ�ʽ  label, group:groupIndex features, ֻ֧�ֶ��� ��֧������,�������Dense����
	};

	enum class ColumnType
	{
		Unknown = 0,
		Feature,
		Name,
		Label,
		Weight,
		Attribute,
		Category,  //�ݲ�֧��
		Text,       //�ݲ�֧��
	};

	//����header �Լ������� instance�ڲ����ݵ� ������Ϣ �������� Instances����ֻҪschema��data
	class HeaderSchema
	{
	public:
		~HeaderSchema() = default;
		HeaderSchema() = default;
		HeaderSchema(HeaderSchema&&) = default;
		HeaderSchema& operator = (HeaderSchema&&) = default;
		HeaderSchema(const HeaderSchema&) = default;
		HeaderSchema& operator = (const HeaderSchema&) = default;
		//bool operator==(const HeaderSchema&) const = default;
		//���ź� û��Ĭ�ϵ�
		bool operator==(const HeaderSchema& other) const
		{
			if (fileFormat == FileFormat::LibSVM && other.fileFormat == FileFormat::LibSVM)
			{
				return true;
			}
			
			return featureNames == other.featureNames && tagNames == other.tagNames;
		}
	public:
		int FeatureNum() const
		{
			return featureNames.size();
		}

		int NumFeatures() const
		{
			return featureNames.size();
		}

		bool HasHeader() const
		{
			return hasHeader;
		}

		string HeaderStr() const
		{
			if (hasHeader)
			{
				return headerStr;
			}
			else
			{
				return "";
			}
		}

		void SetHeader(string header_, bool hasHeader_)
		{
			headerStr = header_;
			hasHeader = hasHeader_;
		}

		FileFormat GetFileFormat() const
		{
			return fileFormat;
		}

	protected:
		friend class cereal::access;
		template<class Archive>
		void serialize(Archive &ar, const unsigned int version)
		{
			ar & CEREAL_NVP(featureNames);
			ar & CEREAL_NVP(attributeNames);
			ar & CEREAL_NVP(tagNames);
			ar & CEREAL_NVP(headerStr);
			ar & CEREAL_NVP(columnTypes);
			ar & CEREAL_NVP(hasHeader);
			ar & CEREAL_NVP(hasWeights);
			ar & CEREAL_NVP(groupKeys);
			ar & CEREAL_NVP(fileFormat);
			ar & CEREAL_NVP(instanceNameHeaderString);
			ar & CEREAL_NVP(normalized);
			ar & CEREAL_NVP(numClasses);
		}

	public:
		FeatureNamesVector featureNames;
		svec attributeNames;
		svec tagNames;
		string headerStr;
		vector<ColumnType> columnTypes; //for writting instances
		bool hasHeader = false;
		bool hasWeights = false;
		//��tlc���� n0,a1�������� ���� ֱ�Ӱ���index user�Լ�����֤������name����attribute��Ӧ��index
		svec groupKeys; //for ranking instances or used in cv folder creationg as stratify key
		FileFormat fileFormat = FileFormat::Unknown;
		string instanceNameHeaderString = "Instance";
		bool normalized = false;
		int numClasses = 2; //�м������� Ĭ��2���� ���< 0���ʾregression�ȷǷ�������
	};

}  //----end of namespace gezi

#endif  //----end of PREDICTION__INSTANCES__HEADER_SCHEMA_H_
