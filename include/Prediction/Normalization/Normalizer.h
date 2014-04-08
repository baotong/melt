/**
 *  ==============================================================================
 *
 *          \file   Prediction/Normalization/Normalizer.h
 *
 *        \author   chenghuige
 *
 *          \date   2014-04-01 17:07:28.184018
 *
 *  \Description:   ��һ���� [0,1]
 *    @TODO TLC���ڿ��ǵ�StreamingIntancesѵ������ ��û���ṩһ��Normalizerֱ��norm���еĽӿ�
 *          ������ѵ�������process prepareȻ�����normalize
 *
 *  ==============================================================================
 */

#ifndef PREDICTION__NORMALIZATION__NORMALIZER_H_
#define PREDICTION__NORMALIZATION__NORMALIZER_H_
#include "common_util.h"
#include "Prediction/Instances/Instances.h"
#include "feature/Feature.h"
namespace gezi {

	class Normalizer
	{
	public:
		Normalizer()
		{
			ParseArgs();
		}

		void ParseArgs()
		{

		}

		//д���ı�
		virtual void Save(string outfile)
		{

		}

		virtual void Load(string infile)
		{

		}

		virtual string Name()
		{
			return "Not specified";
		}

		/// Process next initialization example
		virtual void Process(const Vector& vec)
		{

		}

		//TLC �Ƚϸ��� ���Ҹо���bug  ������dense sparse��ת������ 
		//ͬʱ��norm֮�󸲸�ԭ��� �����Ҫ 
		template<typename _Vector>
		void Normalize(_Vector& vec)
		{
			NormalizeCore(vec);
		}

		void Normalize(Instance& instance)
		{
			NormalizeCore(instance.features);
		}

		void Normalize(const InstancePtr& instance)
		{
			NormalizeCore(instance->features);
		}

		//����norm ��μ���Feature 1.Template For Normalzier class ? 2. Feature : public Vector add names field��Ҫ�޸�feature_util.h ��дFeature��
		//Feature��2��vector����1��vector<Node> InverteIndex ��Ҫ��Node ����洢
		virtual void NormalizeCore(Vector& vec)
		{

		}

		//@TODO better handle? ģ���Ա�����޷��麯�� ��������Ԥ�� ������Կ���ȥ��Feature��
		//��дһ��FeatureVector : public Vector 
		//����ԭ��Vector������ ����һ��names���� Vector���ֲ���sparse��ʾ
		//�޸�feature_util.h -> feature_vector_util.h ��������ForEach����
		virtual void NormalizeCore(Feature& vec)
		{

		}

		///norm���
		//����Fast��׺���ǳ���ⷨȫ������ �����ı������������Ŀ�� ����ϡ���ٶȽ��� ��Ҫ������֤Fast�ӿڵ���ȷ��
		//����Func ������Ĳ��ֺ������麯��
		template<typename Func>
		void Normalize(Vector& vec, Func func)
		{
			if (vec.IsDense())
			{
				//NormalizeDense(vec, func);
				NormalizeDenseFast(vec, func);
			}
			else
			{
				//NormalizeSparse(vec, func);
				NormalizeSparseFast(vec, func);
			}
			vec.normalized = true;
		}

		//���� ������sparse���� ��һ��
		template<typename Func>
		void Normalize(Feature& feature, Func func)
		{
			NormalizeSparseFast(feature, func);
			feature.normalized = true;
		}

		template<typename Func>
		void NormalizeDense(Vector& vec, Func func)
		{
			vec.ForEachDense([&func](int index, Float& value)
			{
				func(index, ref(value));
			});
		}

		template<typename Func>
		void NormalizeSparse(Vector& vec, Func func)
		{
			Vector result(_featureNum);
			vec.ForEachAllSparse([&func, &result](int index, Float value)
			{
				Float val = value;
				func(index, ref(val));
				result.Add(index, val);
			});
			vec.Swap(result);
		}

		template<typename Func>
		void NormalizeDenseFast(Vector& vec, Func func)
		{
			for (int index : _morphIndices)
			{
				func(index, vec.Values()[index]);
			}
		}

		//@TODO can use ForEach ?
		template<typename Func, typename _Vector>
		void NormalizeSparseFast(_Vector& vec, Func func)
		{
			_Vector result(_featureNum); //@NOTICE һ��ע��Ҫ���ϳ��ȹ���
			int len = _shiftIndices.size();
			int len2 = vec.Count();

			Float val;
			int index, index2;
			int i = 0, j = 0;
			for (; i < len && j < len2;)
			{
				index = _shiftIndices[i];
				index2 = vec.Index(j);
				if (index == index2)
				{
					val = vec.Value(j);
					func(index, ref(val));
					result.Add(index, val);
					i++;
					j++;
				}
				else if (index < index2)
				{
					val = 0;
					func(index, ref(val));
					result.Add(index, val);
					i++;
				}
				else
				{
					val = vec.Value(j);
					func(index2, ref(val));
					result.Add(index2, val);
					j++;
				}
			}
			for (; i < len; i++)
			{
				index = _shiftIndices[i];
				val = 0;
				func(index, ref(val));
				result.Add(index, val);
			}

			for (; j < len2; j++)
			{
				index2 = vec.Index(j);
				val = vec.Value(j);
				func(index2, ref(val));
				result.Add(index2, val);
			}

			//vec.Swap(result);
			vec = move(result);
		}

		/// Begin iterating through initialization examples
		virtual void Begin()
		{

		}

		/// Finish processing initialization examples; ready to normalize
		virtual void Finalize()
		{

		}

		void Prepare(const Instances& instances)
		{
			Noticer nt("Normalize prepare");
			_featureNum = instances.FeatureNum();
			_featureNames = instances.FeatureNames();
			Begin();
			for (uint64 i = 0; i < instances.Size(); i++)
			{
				if (i == _maxNormalizationExamples)
				{
					break;
				}
				Process(instances[i]->features);
			}
			Finalize();
		}

		//@TODO Load info or just test data normalize
		void PrepareAndNormalize(Instances& instances)
		{
			Prepare(instances);
			Normalize(instances);
		}
		void Normalize(Instances& instances)
		{
			Noticer nt("Normalize");
#pragma omp parallel for //omp not work for foreach loop ? @TODO
			for (uint64 i = 0; i < instances.Size(); i++)
			{
				Normalize(instances[i]->features);
			}
		}
		bool Trunct() const 
		{
			return _trunct;
		}

		//ѡ��set�����Ǻ�����Normalize(vec, trunct=true)ѡ�� ��Ϊ���trunct=true��Ӧ���Ƕ����ж��� 
		//��������Normalizer���ڲ����� �������ⲿ���ı��
		void SetTrunct(bool trunct)
		{ 
			_trunct = trunct;
		}
	protected:
		int _featureNum;
		svec _featureNames;
		ivec _morphIndices;
		ivec _shiftIndices;
		//----------------------------args begin
		//|if feature is out of bounds, threshold at 0/1, or return values below 0 and above 1?
		bool _trunct = false; //@TODO here or in MinMaxNormalizer GuassianNormalzier need it?
		uint64 _maxNormalizationExamples = 1000000;//numNorm|
		//-----------------------------args end
	private:

	};

	typedef shared_ptr<Normalizer> NormalizerPtr;
}  //----end of namespace gezi

#endif  //----end of PREDICTION__NORMALIZATION__NORMALIZER_H_
