#include "data.hh"
#include "maxent.hh"

#include <iostream>
#include <string>

#define DEBUG 0

using namespace std;

int main(int argc, char *argv[])
{
	if (argc < 2) {
		cerr << "Usage: " << argv[0] << " lambda1 [lambda2...] < data > weights" << endl;
		return 1;
	}

	DataSet data(cin);

#if DEBUG
	data.dump(cerr);
#else
	cerr << "Data loaded:"
	     << " train " << data.train.size()
	     << " dev " << data.dev.size()
	     << " test " << data.test.size()
	     << endl;
#endif


	if (argc > 2 && data.dev.size() == 0) {
		cerr << "No dev data; cannot tune lambda." << endl;
		return 1;
	}


	if (argc == 2) {
		MaxEntModel model;
		model.train(data, stod(argv[1]));
		model.dump_model(cout);
	} else {
		MaxEntModel model, best_model;
		size_t dev_total = data.dev.size();
		size_t max_correct = 0;
		double max_lambda = 0;
		for (int i = 1; i < argc; ++i) {
			double lambda = stod(argv[i]);
			model.train(data, lambda);
			size_t correct = model.test(data.dev);
			cerr << "lambda = " << lambda
			     << " error rate = "
			     << (dev_total - correct) << " / " << dev_total
			     << " = "
			     << (dev_total - correct) / static_cast<double>(dev_total)
			     << endl;
			if (correct > max_correct) {
				max_correct = correct;
				max_lambda = lambda;
				best_model = model;
			}
		}
		cerr << "best lambda = " << max_lambda
		     << " error rate = "
		     << (dev_total - max_correct) << " / " << dev_total
		     << " = "
		     << (dev_total - max_correct) / static_cast<double>(dev_total)
		     << endl;
		best_model.dump_model(cout);
	}

	return 0;
}
