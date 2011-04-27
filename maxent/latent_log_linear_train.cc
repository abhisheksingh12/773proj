#include "data.hh"
#include "latent_log_linear.hh"

#include <iostream>
#include <string>

#define DEBUG 0

using namespace std;

int main(int argc, char *argv[])
{
	if (argc < 3) {
		cerr << "Usage: " << argv[0] << " H lambda1 [lambda2...] < data > weights" << endl;
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


	if (argc > 3 && data.dev.size() == 0) {
		cerr << "No dev data; cannot tune lambda." << endl;
		return 1;
	}

	size_t latent_count = stoi(argv[1]);

	cerr << "latent_count = " << latent_count << endl;

	if (argc == 3) {
		LatentLogLinearModel model;
		model.train(data, latent_count, stod(argv[2]));
		model.dump_model(cout);
	} else {
		LatentLogLinearModel model, best_model;
		size_t dev_total = data.dev.size();
		size_t max_correct = 0;
		double max_lambda = 0;
		for (int i = 2; i < argc; ++i) {
			double lambda = stod(argv[i]);
			model.train(data, latent_count, lambda);
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
