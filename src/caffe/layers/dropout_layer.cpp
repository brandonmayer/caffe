// Copyright 2013 Yangqing Jia

#include <vector>

#include "caffe/common.hpp"
#include "caffe/layer.hpp"
#include "caffe/syncedmem.hpp"
#include "caffe/vision_layers.hpp"

namespace caffe {

template <typename Dtype>
void DropoutLayer<Dtype>::SetUp(const vector<Blob<Dtype>*>& bottom,
      vector<Blob<Dtype>*>* top) {
  NeuronLayer<Dtype>::SetUp(bottom, top);
  // Set up the cache for random number generation
  rand_vec_.reset(new SyncedMemory(bottom[0]->count() * sizeof(int)));
  threshold_ = this->layer_param_.dropout_ratio();
  DCHECK(threshold_ > 0.);
  DCHECK(threshold_ < 1.);
  scale_ = 1. / (1. - threshold_);
  uint_thres_ = (unsigned int)(UINT_MAX * threshold_);
}

template <typename Dtype>
void DropoutLayer<Dtype>::Forward_cpu(const vector<Blob<Dtype>*>& bottom,
    vector<Blob<Dtype>*>* top) {
  const Dtype* bottom_data = bottom[0]->cpu_data();
  Dtype* top_data = (*top)[0]->mutable_cpu_data();
  int* mask = reinterpret_cast<int*>(rand_vec_->mutable_cpu_data());
  const int count = bottom[0]->count();
  if (Caffe::phase() == Caffe::TRAIN) {
    // Create random numbers
    viRngBernoulli(VSL_RNG_METHOD_BERNOULLI_ICDF, Caffe::vsl_stream(),
        count, mask, 1. - threshold_);
    for (int i = 0; i < count; ++i) {
      top_data[i] = bottom_data[i] * mask[i] * scale_;
    }
  } else {
    memcpy(top_data, bottom_data, bottom[0]->count() * sizeof(Dtype));
  }
}

template <typename Dtype>
Dtype DropoutLayer<Dtype>::Backward_cpu(const vector<Blob<Dtype>*>& top,
    const bool propagate_down,
    vector<Blob<Dtype>*>* bottom) {
  CHECK(Caffe::phase() == Caffe::TRAIN);
  if (propagate_down) {
    const Dtype* top_diff = top[0]->cpu_diff();
    Dtype* bottom_diff = (*bottom)[0]->mutable_cpu_diff();
    const int* mask = reinterpret_cast<const int*>(rand_vec_->cpu_data());
    const int count = (*bottom)[0]->count();
    for (int i = 0; i < count; ++i) {
      bottom_diff[i] = top_diff[i] * mask[i] * scale_;
    }
  }
  return Dtype(0);
}


INSTANTIATE_CLASS(DropoutLayer);


}  // namespace caffe
