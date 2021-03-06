/**
 *  ==============================================================================
 *
 *          \file   Prediction/Normalization/AffineNormalizer.h
 *
 *        \author   chenghuige
 *
 *          \date   2014-04-01 17:07:57.638996
 *
 *  \Description:   @TODO 总结归一化的速度优化技巧写博客。。
 *  ==============================================================================
 */

#ifndef PREDICTION__NORMALIZATION__AFFINE_NORMALIZER_H_
#define PREDICTION__NORMALIZATION__AFFINE_NORMALIZER_H_
#include "Prediction/Normalization/Normalizer.h"
namespace gezi {

	class AffineNormalizer : public Normalizer
	{
	public:
		virtual ~AffineNormalizer() {}
		//AffineNormalizer()
		//{
		//	_func = [this](int index, Float& value)
		//	{ //index >= _numFeatures 只有在train-test模式 读取最大index不同的libsvm或者其它没有指定最大特征数目的文件可能
		//		if (index >= _numFeatures || _scales[index] <= 0)
		//		{//保持不变 Dense Sparse 保持逻辑一致 如果置为0 
		//			//需要AffineInit函数中shiftIndices增加scale <=0 且offset!=0的点
		//			//value = .0;
		//			return;
		//		}
		//		else if (_trunct)
		//		{
		//			if (value >= _offsets[index] + _scales[index])
		//				value = 1.0;
		//			else if (value <= _offsets[index])
		//				value = 0.0;
		//			else
		//				value = (value - _offsets[index]) / _scales[index];
		//		}
		//		else
		//		{
		//			value = (value - _offsets[index]) / _scales[index];
		//		}
		//	};
		//}

		AffineNormalizer()
		{
			_func = [this](int index, Float& value)
			{ //index >= _numFeatures 只有在train-test模式 读取最大index不同的libsvm或者其它没有指定最大特征数目的文件可能
				if (index >= _numFeatures || _scales[index] <= 0)
				{//保持不变 Dense Sparse 保持逻辑一致 如果置为0 
					//需要AffineInit函数中shiftIndices增加scale <=0 且offset!=0的点
					value = .0; //为了VW不能正常运行 train test norm模式
					return;
				}
				else if (_trunct)
				{
					if (value >= _offsets[index] + _scales[index])
						value = 1.0;
					else if (value <= _offsets[index])
						value = 0.0;
					else
						value = (value - _offsets[index]) / _scales[index];
				}
				else
				{
					value = (value - _offsets[index]) / _scales[index];
				}
			};
		}

		virtual bool LoadText(string infile) override
		{
			svec lines = read_lines(infile);
			if (lines.empty())
				return false;
			int idx = 0;
			CHECK_EQ(parse_string_param("NormalizerType=", lines[idx++]), Name());
			_trunct = parse_bool_param("Trunct=", lines[idx++]);
			_numFeatures = parse_int_param("FeatureNum=", lines[idx++]);
			_offsets.resize(_numFeatures);
			_scales.resize(_numFeatures);
			for (int i = 0; i < _numFeatures; i++)
			{
				string offset, scale;
				split(lines[i + idx], '\t', offset, scale);
				_offsets[i] = DOUBLE(offset);
				_scales[i] = DOUBLE(scale);
			}
			AffineInit();
			return true;
		}

		virtual void SaveText(string outfile) override
		{
			LoadSave::SaveText(outfile);
			ofstream ofs(outfile);
			ofs << "NormalizerName=" << Name() << endl;
			ofs << "Trunct=" << (int)_trunct << endl;
			ofs << "FeatureNum=" << _numFeatures << endl;
			for (int i = 0; i < _numFeatures; i++)
			{
				ofs << _offsets[i] << "\t" << _scales[i] << endl;
			}
		}

		virtual void Save(string path) override
		{
			LoadSave::Save(path);
			serialize_util::save(*this, path);
		}

		virtual bool Load(string path) override
		{
			bool ret = true;
			ret = Normalizer::Load(path);
			if (!ret)
				return false;
			ret = serialize_util::load(*this, path);
			return ret;
		}

		virtual void Begin() override
		{
			_offsets.resize(_numFeatures, 0);
			_scales.resize(_numFeatures, 0);
		}

		virtual void Finish() override
		{

		}

		//void AffineInit()
		//{
		//	for (int i = 0; i < _numFeatures; i++)
		//	{
		//		if (_scales[i] <= 0)
		//		{ //按照TLC 如果是始终值一样 仍然维持原样 不scale 不置为0 @TODO
		//			//无效特征 始终是6的比如 还是6 不变成0 @TODO 需要置为0？
		//			if (!_featureNames.empty())
		//			{
		//				VLOG(4) << "Feature " << i << " : " << _featureNames[i]
		//					<< " always take value " << _offsets[i];
		//			}
		//		}
		//		else if (_offsets[i] != 0.0)
		//		{ //如果最小值是0 那么 所有的点都只需要morph不需要shift，同时如果是0值点 不需要变化,dense 使用morphIndices, sparse使用shiftIdices 就是求并集 非0 union 需要moph的0值点
		//			_shiftIndices.push_back(i);
		//			_morphIndices.push_back(i);
		//		}
		//		else if (_scales[i] != 1.0)
		//		{
		//			_morphIndices.push_back(i);
		//		} //like [0,1] 这样的所有点都不需要变化
		//	}

		//	PVEC(_offsets);
		//	PVEC(_scales);
		//	PVEC(_morphIndices);
		//	PVEC(_shiftIndices);
		//}

		void AffineInit()
		{
			for (int i = 0; i < _numFeatures; i++)
			{
				if (_scales[i] <= 0)
				{ //按照TLC 如果是始终值一样 仍然维持原样 不scale 不置为0 @TODO
					//无效特征 始终是6的比如 还是6 不变成0 @TODO 需要置为0？
					if (!_featureNames.empty())
					{
						VLOG(4) << "Feature " << i << " : " << _featureNames[i]
							<< " always take value " << _offsets[i];
					}

					if (_offsets[i] != 0.0)
					{
						_shiftIndices.push_back(i);
					}
				}
				else if (_offsets[i] != 0.0)
				{ //如果最小值是0 那么 所有的点都只需要morph不需要shift，同时如果是0值点 不需要变化,dense 使用morphIndices, sparse使用shiftIdices 就是求并集 非0 union 需要moph的0值点
					_shiftIndices.push_back(i);
					_morphIndices.push_back(i);
				}
				else if (_scales[i] != 1.0)
				{
					_morphIndices.push_back(i);
				} //like [0,1] 这样的所有点都不需要变化
			}

			PVEC(_offsets);
			PVEC(_scales);
			PVEC(_morphIndices);
			PVEC(_shiftIndices);
		}

		friend class boost::serialization::access;
		template<class Archive>
		void serialize(Archive &ar, const unsigned int version)
		{
		/*	ar & boost::serialization::base_object<Normalizer>(*this);
			ar & _offsets;
			ar & _scales;*/
			ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Normalizer);
			ar & GEZI_SERIALIZATION_NVP(_offsets);
			ar & GEZI_SERIALIZATION_NVP(_scales);
		}

	protected:
		Fvec _offsets;
		Fvec _scales;
		vector<uint64> _counts;
	};

}  //----end of namespace gezi
CEREAL_REGISTER_TYPE(gezi::AffineNormalizer);
#endif  //----end of PREDICTION__NORMALIZATION__AFFINE_NORMALIZER_H_
