#include "data.hh"
#include "latent_log_linear.hh"

#include <iostream>
#include <string>

#define DEBUG 0

using namespace std;

int main(int argc, char *argv[])
{
	if (argc < 9) {
		cerr << "Usage: " << argv[0] << " H h y hx hy xy hxy lambda1 [lambda2...] < data > weights" << endl;
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


	if (argc > 9 && data.dev.size() == 0) {
		cerr << "No dev data; cannot tune lambda." << endl;
		return 1;
	}

	size_t latent_count = stoi(argv[1]);

	LLLMStruct st = {
		static_cast<bool>(stoi(argv[2])),
		static_cast<bool>(stoi(argv[3])),
		static_cast<bool>(stoi(argv[4])),
		static_cast<bool>(stoi(argv[5])),
		static_cast<bool>(stoi(argv[6])),
		static_cast<bool>(stoi(argv[7]))
	};

	if (argc == 9) {
		LatentLogLinearModel model;
		model.train(data, st, latent_count, stod(argv[8]));
		model.dump_model(cout);
	} else {
		LatentLogLinearModel model, best_model;
		size_t dev_total = data.dev.size();
		size_t max_correct = 0;
		double max_lambda = 0;
		for (int i = 8; i < argc; ++i) {
			double lambda = stod(argv[i]);
			model.train(data, st, latent_count, lambda);
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
