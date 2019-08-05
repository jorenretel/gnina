/*
 * cnn_scorer.h
 *
 *  Created on: May 2, 2016
 *      Author: dkoes
 */

#ifndef SRC_LIB_CNN_SCORER_H_
#define SRC_LIB_CNN_SCORER_H_

#include "caffe/caffe.hpp"
#include "caffe/net.hpp"
#include "caffe/layer.hpp"
#include "caffe/layers/molgrid_data_layer.hpp"
#include "boost/thread/mutex.hpp"
#include <boost/thread/recursive_mutex.hpp>

#include "model.h"
#include "cnn_data.h"

/* This class evaluates protein-ligand poses according to a provided
 * Caffe convolutional neural net (CNN) model.
 */

class CNNScorer {
  public:
    typedef float Dtype;
  private:
    caffe::shared_ptr<caffe::Net<Dtype> > net;
    caffe::MolGridDataLayer<Dtype> *mgrid;
    caffe::MolGridDataParameter *mgridparam;
    cnn_options cnnopts;

    caffe::shared_ptr<boost::recursive_mutex> mtx; //todo, enable parallel scoring

    //scratch vectors to avoid memory reallocation
    std::vector<gfloat3> gradient;
    std::vector<gfloat3> atoms;
    std::vector<short> channels;
    vec current_center; //center last time set_center was called, if min frame is moving, the mgrid center will be changing

    // Receptor and ligand information
    std::vector<float3> ligand_coords, receptor_coords;
    std::vector<smt> ligand_smtypes, receptor_smtypes;

    std::size_t num_flex_atoms; // Number of flexible atoms

    // Set ligand and receptor atoms and coordinates from model
    void setLigand(const model& m);
    void setReceptor(const model& m);

    void getGradient();

  public:
    CNNScorer()
        : mgrid(NULL), mtx(new boost::recursive_mutex), current_center(NAN,NAN,NAN) {
    }
    virtual ~CNNScorer() {
    }

    CNNScorer(const cnn_options& opts);

    bool initialized() const {
      return net.get();
    }

    bool has_affinity() const; //return true if can predict affinity

    float score(model& m); //score only - no gradient
    float score(model& m, bool compute_gradient, float& affinity, float& loss);

    void outputDX(const std::string& prefix, double scale = 1.0, bool relevance =
        false, std::string layer_to_ignore = "", bool zero_values = false);
    void outputXYZ(const std::string& base, const std::vector<gfloat3>& atoms,
        const std::vector<short>& whichGrid, const std::vector<gfloat3>& gradient);
    std::unordered_map<std::string, float> get_gradient_norm_per_atom(bool receptor);
    std::unordered_map<std::string, float> get_relevance_per_atom(bool receptor);

    void lrp(const model& m, const std::string& layer_to_ignore = "",
        bool zero_values = false);
    void gradient_setup(const model& m, const std::string& recname,
        const std::string& ligname, const std::string& layer_to_ignore = "");

    //readjust center
    void set_center_from_model(model &m);

    const cnn_options& options() const {
      return cnnopts;
    }

    vec get_center() const {
      return current_center;
    }
    fl get_grid_dim() const {
      return mgrid->getDimension();
    }
    fl get_grid_res() const {
      return mgrid->getResolution();
    }

    caffe::MolGridDataLayer<Dtype> * get_mgrid() { return mgrid; }
  protected:
    void get_net_output(Dtype& score, Dtype& aff, Dtype& loss);
    void check_gradient();
};

#endif /* SRC_LIB_CNN_SCORER_H_ */
