#include <float.h>

#include "reductions.h"

using namespace LEARNER;

namespace Scorer {
  struct scorer{
    vw* all;
  };
  
  template <bool is_learn, float (*link)(float in)>
  void predict_or_learn(scorer& s, learner& base, example& ec)
  {
    label_data* ld = (label_data*)ec.ld;
    s.all->set_minmax(s.all->sd, ld->label);

    if (is_learn && ld->label != FLT_MAX && ld->weight > 0)
      base.learn(ec);
    else
      base.predict(ec);
    
    if(ld->weight > 0 && ld->label != FLT_MAX)
      ec.loss = s.all->loss->getLoss(s.all->sd, ld->prediction, ld->label) * ld->weight;

    ld->prediction = link(ld->prediction);
  }

  float logistic(float in)
  {
    return 1.f / (1.f + exp(- in));
  }

  float noop(float in)
  {
    return in;
  }

  learner* setup(vw& all, po::variables_map& vm)
  {
    scorer* s = (scorer*)calloc_or_die(1, sizeof(scorer));
    s->all = &all;

    po::options_description link_opts("Link options");
    
    link_opts.add_options()
      ("link", po::value<string>()->default_value("identity"), "Specify the link function: identity or logistic");

    vm = add_options(all, link_opts);

    learner* l = new learner(s, all.l);

    string link = vm["link"].as<string>();
    if (!vm.count("link") || link.compare("identity") == 0)
      {      
	l->set_learn<scorer, predict_or_learn<true, noop> >();
	l->set_predict<scorer, predict_or_learn<false, noop> >();
      }
    else if (link.compare("logistic") == 0)
      {
	all.file_options.append(" --link=logistic ");
	l->set_learn<scorer, predict_or_learn<true, logistic> >();
	l->set_predict<scorer, predict_or_learn<false, logistic> >();
      }
    else 
      {
	cerr << "Unknown link function: " << link << endl;
	throw exception();
      }

    return l;
  }
}
