/**
*  ==============================================================================
*
*          \file   Prediction/Instances/InstanceParser.h
*
*        \author   chenghuige
*
*          \date   2014-03-27 17:47:57.438203
*
*  \Description:   �����������ļ� ������dense ���� sparse��ʽ �����Instances
*                  @TODOĿǰֻ����Instances�������鶼�������ڴ� ������ StreamingInstances @TODO
* �������ݸ�ʽ dense (default)
*  //with header
*	#id label age weight
*  _123 0    32   18
*  //whithout header
*  _123 0  32 18 //id label feature1 feature2
* �������ݸ�ʽ sparse (default)
*   0 5 0:0.4 3:0.1
*   1 5 1:0.3
* �ڲ����ݴ����TLCһ�� ����������dense����sparse�����жϵ�ǰ0����Ŀ ���> 1/2 * FeatureNum ��dense����sparse
*  ==============================================================================
*/

#ifndef PREDICTION__INSTANCES__INSTANCE_PARSER_H_
#define PREDICTION__INSTANCES__INSTANCE_PARSER_H_

#include "common_util.h"
#include "Prediction/Instances/Instances.h"
#include "RegexSearcher.h"

namespace gezi {

	enum class ColumnType
	{
		Feature = 0,
		Name,
		Label,
		Weight,
		Attribute,
		Category
	};

	class InstanceParser
	{
	public:
		struct Arguments
		{
			int labelIdx = -1;
			int weightIdx = -1;
			//nameIdx|, seprated like 1,2,3, name filed will be shown in .inst.txt result file
			string namesIdx = "";
			//|the same as nameIdx, attrIdx will be filed to be ignored
			string attrsIdx = "";
			bool hasHeader = false; //|header: no header by default
			string sep = "tab"; //|or space or something like , ; etc. 
			string ncsep = "|"; //|contact names filed like pid|title|content 4003|good title|good content
			//|excl vs. incl determines whether features for the expression below are included or excluded. expression=[s:substringList | r:regexList | i:indexList | e:exactNameList]
			string excl = "";
			//|use excl will exlude those specified, use incl will only include those specified, use incl + excl means first incl then excl
			string incl = "";
			bool keepSparse = false; //sparse|
			bool keepDense = false; //dense|
		};

		InstanceParser()
		{
			ParseArguments();
			InitParam();
		}

		void ParseArguments();

		Arguments& Args()
		{
			return _args;
		}

		ivec GetIndexesFromInput(string input)
		{
			ivec result;
			svec sv = split(input, ",");
			for (auto item : sv)
			{
				if (contains(item, "-"))
				{
					svec t = split(item, "-");
					int start = INT(t[0]);
					int end = INT(t[1]) + 1;
					for (int i = start; i < end; i++)
					{
						result.push_back(i);
					}
				}
				else
				{
					result.push_back(INT(item));
				}
			}
		}
		void InitParam()
		{
			if (_args.sep == "tab")
			{
				_sep = "\t";
			}
			else if (_args.sep == "space")
			{
				_sep = " ";
			}
			else if (_args.sep.empty())
			{
				_sep = "\t";
			}
			else
			{
				_sep = _args.sep;
			}

			if (!_args.attrsIdx.empty())
			{
				_attributesIdx = GetIndexesFromInput(_args.attrsIdx);
				PVEC(_attributesIdx);
			}

			if (!_args.namesIdx.empty())
			{
				_namesIdx = GetIndexesFromInput(_args.namesIdx);
				PVEC(_namesIdx);
			}
		}

		//@TODO ��ǰֵ����֧��һ�� ���� i, e, s, r�����Ƕ��ͬʱ����,һ�� ��õ�r ����ƥ�伴��
		ivec GetSelectedFeatures(string input)
		{
			string pattern, content;
			if (contains(input, ":"))
			{
				pattern = input.substr(0, 1);
				content = input.substr(2);
			}
			else
			{
				pattern = "r";
				content = input;
			}
			if (pattern == "i")
			{
				return GetIndexesFromInput(content);
			}
			ivec result;
			svec t = split(content, ",");
			set<string> s(t.begin(), t.end());
			if (pattern == "e")
			{ //exact
				for (int i = 0; i < _featureNum; i++)
				{
					if (s.count(_instances.schema.featureNames[i]))
					{
						result.push_back(i);
					}
				}
			}
			else if (pattern == "s")
			{ //substr
				for (int i = 0; i < _featureNum; i++)
				{
					for (auto p : s)
					{
						if (contains(_instances.schema.featureNames[i], p))
						{
							result.push_back(i);
						}
					}
				}
			}
			else if (pattern == "r")
			{ //regex
				RegexSearcher rs;
				rs.add(s);
				for (int i = 0; i < _featureNum; i++)
				{
					if (rs.has_match(_instances.schema.featureNames[i]))
					{
						result.push_back(i);
					}
				}
			}
			else
			{
				LOG(FATAL) << "Unsupported pattern " << input << " only support i: e: s: r:";
			}
			return result;
		}

		BitArray GetSelectedArray()
		{
			BitArray filterArray;
			if (!_args.incl.empty())
			{
				ivec incls = GetSelectedFeatures(_args.incl);

				if (incls.empty())
				{
					LOG(WARNING) << "Do not has any match for incl " << _args.incl;
				}
				else
				{
					filterArray.resize(_featureNum, false);
					for (int idx : incls)
					{
						LOG(INFO) << "Including feature: " << _instances.schema.featureNames[idx];
						filterArray[idx] = true;
					}
				}
			}

			if (!_args.excl.empty())
			{
				ivec excls = GetSelectedFeatures(_args.excl);

				if (excls.empty())
				{
					LOG(WARNING) << "Do not has any match for excl " << _args.excl;
				}
				else
				{
					if (filterArray.empty())
					{
						filterArray.resize(_featureNum, true);
					}
					for (int idx : excls)
					{
						LOG(INFO) << "Excluding feature: " << _instances.schema.featureNames[idx];
						filterArray[idx] = false;
					}
				}
			}

			if (filterArray.empty())
			{
				filterArray.resize(_featureNum, true);
			}

			return filterArray;
		}
		bool IsSparse()
		{
			return _instanceType == InstanceType::Sparse;
		}
		bool IsDense()
		{
			return _instanceType == InstanceType::Dense;
		}
		void InitColumnTypes(svec& lines)
		{
			for (int idx : _attributesIdx)
			{
				_columnTypes[idx] = ColumnType::Attribute;
			}

			for (int idx : _namesIdx)
			{
				_columnTypes[idx] = ColumnType::Name;
			}

			if (_args.weightIdx >= 0)
			{
				_columnTypes[_args.weightIdx] = ColumnType::Weight;
				_hasWeight = true;
			}

			if (_args.labelIdx >= 0)
			{
				_columnTypes[_args.labelIdx] = ColumnType::Label;
				_labelIdx = _args.labelIdx;
			}

			if (startswith(_firstColums[0], "#"))
			{
				_hasHeader = true;
				_columnTypes[0] = ColumnType::Name;

				if (_labelIdx < 0)
				{
					_labelIdx = 1;
					_columnTypes[1] = ColumnType::Label;
				}
			}

			//ǰ��ȷ�����Ƿ���header
			if (_hasHeader)
			{
				_headerColums.swap(_firstColums);
				_firstColums = split(lines[_hasHeader], _sep);
			}

			if (startswith(_firstColums[0], "_"))
			{
				{
					_columnTypes[0] = ColumnType::Name;
					if (_labelIdx < 0)
					{
						_labelIdx = 1;
						_columnTypes[1] = ColumnType::Label;
					}
				}
			}

			if (_labelIdx < 0)
			{
				_labelIdx = 0;
				_columnTypes[0] = ColumnType::Label;
			}
		}

		void InitNames()
		{
			if (_hasHeader)
			{
				for (int i = 0; i < _columnNum; i++)
				{
					switch (_columnTypes[i])
					{
					case ColumnType::Feature:
						_instances.schema.featureNames.push_back(_headerColums[i]);
						break;
					case ColumnType::Attribute:
						_instances.schema.attributeNames.push_back(_headerColums[i]);
						break;
					case ColumnType::Name:
						_instances.schema.tagNames.push_back(_headerColums[i]);
						break;
					default:
						break;
					}
				}
				_featureNum = _instances.FeatureNum();
			}
			else
			{
				int i = 0;
				if (_instanceType == InstanceType::Dense)
				{
					for (auto type : _columnTypes)
					{
						if (type == ColumnType::Feature)
						{
							string name = "f" + STR(i);
							_instances.schema.featureNames.push_back(name);
							i++;
						}
					}
					_featureNum = _instances.FeatureNum();
				}
				else
				{
					for (int i = 0; i < _featureNum; i++)
					{
						string name = "f" + STR(i);
						_instances.schema.featureNames.push_back(name);
					}
				}
			}
		}

		void CreateInstancesFromDenseFormat(svec& lines, uint64 start)
		{
			uint64 end = start + _instanceNum;
#pragma omp parallel for 
			for (uint64 i = start; i < end; i++)
			{
				string line = lines[i];
				_instances[i - start] = make_shared<Instance>(_featureNum);
				Instance& instance = *_instances[i - start];
				Vector& features = instance.features;
				features.PrepareDense();
				svec l = split(line, _sep);
				CHECK_EQ(l.size(), _columnNum) << "has bad line"; //�������л�������
				int fidx = 0;
				double value = 0;
				for (int j = 0; j < _columnNum; j++)
				{
					string item = l[j];
					switch (_columnTypes[j])
					{
					case ColumnType::Feature:
						value = _selectedArray[fidx++] ? DOUBLE(item) : 0;
						features.Add(value);
						break;
					case ColumnType::Name:
						instance.names.push_back(item);
						break;
					case ColumnType::Label:
						instance.label = DOUBLE(item);
						break;
					case ColumnType::Weight:
						instance.weight = DOUBLE(item);
					default:
						break;
					}
				}

				if (_args.keepSparse)
				{
					features.ToSparse();
				}
				else if (!_args.keepDense)
				{//if not keep dense   �����0ֵ��Ŀ<= FeatureNum/2ת sparse
					features.Sparsify();
				}

				if (!instance.names.empty())
				{
					if (startswith(instance.names[0], "_"))
						instance.names[0] = instance.names[0].substr(1);
					instance.name = join(instance.names, _args.ncsep);
				}
			}
		}

		//@TODO Instance Next(string line) ֧��streaming
		//@TODO tlcò�ƿ�ܶ�thread.feature.txt 9w instance 
		//1000ms ������Ҫ2000ms����Ϊsplit c#���ٶȸ��죿 ����ע����char split find��Ҫ��string
		//����ʹ��omp���к� 200ms�͸㶨
		void CreateInstancesFromSparseFormat(svec& lines, uint64 start)
		{
			uint64 end = start + _instanceNum;
#pragma omp parallel for 
			for (uint64 i = start; i < end; i++)
			{
				string line = lines[i];
				_instances[i - start] = make_shared<Instance>(_featureNum);
				Instance& instance = *_instances[i - start];
				Vector& features = instance.features;
				svec l = split(line, _sep);
				
				for (size_t j = 0; j < l.size(); j++)
				{
					string item = l[j];
					string index_, value_; 
					bool ret = split(item, ':', index_, value_);
					if (ret)
					{
						int index = INT(index_); Float value = DOUBLE(value_);
						if (_selectedArray[index])
							features.Add(index, value);
					}
					else
					{
						//@TODO ��ʱû�м���ʽ��ȷ�� Ҫ����� ���з�:������ ���ں���
						switch (_columnTypes[j])
						{
						case ColumnType::Name:
							instance.names.push_back(item);
							break;
						case ColumnType::Label:
							instance.label = DOUBLE(item);
							break;
						case ColumnType::Weight:
							instance.weight = DOUBLE(item);
							break;
						default:
							break;
						}
					}
				}
				if (_args.keepDense)
				{
					features.ToDense();
				}
				else if (!_args.keepSparse)
				{//if not keep sparse  �����0ֵ��Ŀ> FeatureNum/2תdense
					features.Densify();
				}
				instance.name = join(instance.names, _args.ncsep);
			}
		}

		InstanceType GetInstanceType(string line)
		{
			for (int i = 0; i < _columnNum; i++)
			{
				if (_columnTypes[i] == ColumnType::Feature)
				{
					int j = i + 1;
					while (j < _columnNum && _columnTypes[j] != ColumnType::Feature)
					{
						j++;
					}

					if (j == _columnNum)
					{
						return InstanceType::Dense;
					}
					else
					{
						if (contains(_firstColums[j], ":"))
						{
							_featureNum = INT(_firstColums[i]);
							_columnTypes[i] = ColumnType::Attribute;
							return InstanceType::Sparse;
						}
						else
						{
							return InstanceType::Dense;
						}
					}
				}
			}
			LOG(FATAL) << "Data format wrong not feature field " << line;
		}

		void SetHeaderSchema(string line)
		{
			_instances.SetHeader(line, _hasHeader);
			_instances.schema.instanceType = _instanceType;
			_instances.schema.hasWeights = _hasWeight;
		}

		//��ȡ����Ϣ�����֣���dense����sparse��ʾ
		void ParseFirstLine(svec lines)
		{
			string line = lines[0];
			_firstColums = split(line, _sep + " ");
			_columnNum = _firstColums.size();
			Pval(_columnNum);
			if (_columnNum < 2)
			{
				LOG(FATAL) << "The header must at least has label and one feature";
			}
			_columnTypes.resize(_columnNum, ColumnType::Feature);
			InitColumnTypes(lines);
			_instanceType = GetInstanceType(lines[_hasHeader]);
			InitNames();
			SetHeaderSchema(line);
		}

		void PrintInfo()
		{
			Pval(_instances.FeatureNum());
			Pval(_instances.Count());
			PVEC(_instances.schema.tagNames);
			PVEC(_instances.schema.attributeNames);
			PVAL(IsDense());
			if (IsDense())
			{
				PVEC(_instances.schema.featureNames);
			}
			else
			{
				PVEC_TOPN(_instances.schema.featureNames, 10);
			}
			PVAL(_args.keepSparse);
			PVAL(_args.keepDense);
		}

		Instances&& Parse(const string& dataFile)
		{
			Timer timer;
			vector<string> lines = read_lines(dataFile);
			Pval_(timer.elapsed_ms(), "read_lines");
			if (lines.empty())
			{
				LOG(FATAL) << "Fail to load data file! " << dataFile << " is empty!";
			}
			int instanceNum = lines.size();

			if (_args.hasHeader)
			{
				instanceNum--;
				if (!instanceNum)
				{
					LOG(FATAL) << "Only header no data! " << dataFile;
				}
				_hasHeader = true;
			}

			timer.restart();
			ParseFirstLine(lines);
			Pval_(timer.elapsed_ms(), "ParseFirstLine");

			timer.restart();
			_selectedArray = GetSelectedArray();

			_instanceNum = lines.size() - _hasHeader;
			_instances.resize(_instanceNum, nullptr); //@TODO

			Pval_(timer.elapsed_ms(), "GetSelectedArray");

			timer.restart();
			if (_instanceType == InstanceType::Dense)
			{
				CreateInstancesFromDenseFormat(lines, _hasHeader);
			}
			else
			{
				CreateInstancesFromSparseFormat(lines, _hasHeader);
			}

			Pval_(timer.elapsed_ms(), "CreateInstances");

			PrintInfo();

			return move(_instances);
		}

	protected:
	private:
		Instances _instances;
		uint64 _instanceNum = 0;
		int _featureNum = 0;
		bool _hasHeader = false;
		bool _hasWeight = false;
		InstanceType _instanceType = InstanceType::Dense;

		svec _firstColums;
		svec _headerColums;

		BitArray _selectedArray;
		vector<ColumnType> _columnTypes;
		int _columnNum;
		int _labelIdx = -1;

		//----params
		Arguments _args;
		string _sep;
		ivec _namesIdx;
		ivec _attributesIdx;
	};

}  //----end of namespace gezi

#endif  //----end of PREDICTION__INSTANCES__INSTANCE_PARSER_H_
