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
#include "Numeric/Vector/vector_util.h"
#include "feature/Feature.h" //@TODO remove
namespace gezi {

	class Normalizer
	{
	public:
		enum class NormType
		{
			Affine,
			Bin
		};
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
			Normalize(vec, _func);
		}

		template<typename _Vector>
		_Vector NormalizeCopy(_Vector& vec)
		{
			_Vector temp = vec;
			Normalize(temp, _func);
			return temp;
		}

		void Normalize(Instance& instance)
		{
			Normalize(instance.features, _func);
		}

		Instance NormalizeCopy(const Instance& instance)
		{
			Instance temp = instance;
			Normalize(temp.features, _func);
			return temp;
		}

		InstancePtr NormalizeCopy(InstancePtr instance)
		{
			InstancePtr temp = make_shared<Instance>(*instance);
			Normalize(temp->features, _func); 
			return temp;
		}

		void Normalize(InstancePtr instance)
		{
			Normalize(instance->features, _func);
		}



		/// Begin iterating through initialization examples
		virtual void Begin()
		{

		}

		/// Finish processing initialization examples; ready to normalize
		virtual void Finish()
		{

		}

		void Prepare(const Instances& instances)
		{
			_numFeatures = instances.FeatureNum();
			_featureNames = instances.FeatureNames();
			Begin();
			uint64 len = min((uint64)instances.size(), (uint64)_maxNormalizationExamples);
			ProgressBar pb(Name() + " prepare", len);
			for (uint64 i = 0; i < len; i++)
			{
				pb.progress(i);
				Process(instances[i]->features);
			}
			Finish();
		}

		//@TODO Load info or just test data normalize
		void RunNormalize(Instances& instances)
		{
			Prepare(instances);
			Normalize(instances);
		}
		void Normalize(Instances& instances)
		{
			if (!instances.normalized)
			{
				Noticer nt("Normalize");
#pragma omp parallel for //omp not work for foreach loop ? @TODO
				for (uint64 i = 0; i < instances.Size(); i++)
				{
					Normalize(instances[i]->features);
				}
			}
			instances.normalized = true;
		}

		//@TODO move to MinMax ?
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
		///norm���
		//����Fast��׺���ǳ���ⷨȫ������ �����ı������������Ŀ�� ����ϡ���ٶȽ��� ��Ҫ������֤Fast�ӿڵ���ȷ��
		//����Func ������Ĳ��ֺ������麯��
		template<typename Func>
		void Normalize(Vector& vec, Func func)
		{
			if (!vec.normalized)
			{
				if (_normType == NormType::Affine)
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
				}
				else
				{//bin norm @TODO��ʵ����affine bin ��ʹ��apply(vec, _shiftIndices, func)Ҳ�����ٶ�Ӱ���С
					apply(vec, _shiftIndices, func);
				}
				vec.normalized = true;
			}
		}

		//@TODO remove ���� ������sparse���� ��һ��
		template<typename Func>
		void Normalize(Feature& feature, Func func)
		{
			if (!feature.normalized)
			{
				NormalizeSparseFast(feature, func);
				feature.normalized = true;
			}
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
			Vector result(_numFeatures);
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

		template<typename Func, typename _Vector>
		void NormalizeSparseFast(_Vector& vec, Func func)
		{
			apply_sparse(vec, _shiftIndices, func);
		}
	protected:
		int _numFeatures = 0;
		svec _featureNames;
		ivec _morphIndices; //ֻ��AffineNormalizer��ʹ��
		ivec _shiftIndices; //Affine,Bin��ʹ��
		NormType _normType = NormType::Affine;
		std::function<void(int, Float&)> _func;

		uint64 _numProcessedInstances = 1; //�����instance��Ŀ

		//----------------------------args begin
		//|if feature is out of bounds, threshold at 0/1, or return values below 0 and above 1?
		bool _trunct = false; //@TODO �ƺ�ֻ��MinMax�ſ���Խ���
		uint64 _maxNormalizationExamples = 1000000;//numNorm|
		//-----------------------------args end
	private:

	};

	typedef shared_ptr<Normalizer> NormalizerPtr;
}  //----end of namespace gezi

#endif  //----end of PREDICTION__NORMALIZATION__NORMALIZER_H_
