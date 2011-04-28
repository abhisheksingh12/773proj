#include "latent_log_linear.hh"

#include <fstream>
#include <iostream>
#include <sstream>
#include <ios>
#include <iomanip>

using namespace std;

bool predict_line(const string &line, const LatentLogLinearModel &m, ostream &output)
{
	istringstream strin(line);

	size_t ref_label;
	strin >> ref_label;

	sparse_t sparse = m.build_sparse(strin);

	vec_t probs;
	size_t pred_label = m.predict(sparse, probs);

	output << pred_label;
	for (auto i = probs.begin(); i != probs.end(); ++i)
		output << (i == probs.begin() ? '\t' : ' ')
		       << setiosflags(ios_base::fixed)
		       << setprecision(20)
		       << *i;
	output << '\n';

	return pred_label == ref_label;
}

int main(int argc, char *argv[])
{
	if (argc != 2) {
		cerr << "Usage: " << argv[0] << " weights < input > output" << endl;
		return 1;
	}

	ifstream fin(argv[1]);
	LatentLogLinearModel m;
	m.load_model(fin);

	cerr << "Model loaded: "
	     << "path = " << argv[1] << " "
	     << "label_count = " << m.label_count << " "
	     << "feat_count = " << m.feat_count << " "
	     << "latent_count = " << m.latent_count
	     << endl;

	string line;
	size_t correct = 0;
	size_t total = 0;
	while (getline(cin, line) && line != "DEV" && line != "TEST") {
		correct += predict_line(line, m, cout);
		++total;
	}
	cerr << "Error rate = "
	     << (total - correct) << " / " << total
	     << " = " << (total - correct) / static_cast<double>(total)
	     << endl;

	if (line == "DEV") {
		correct = 0;
		total = 0;
		while (getline(cin, line) && line != "TEST") {
			correct += predict_line(line, m, cout);
			++total;
		}
		cerr << "Dev error rate = "
		     << (total - correct) << " / " << total
		     << " = " << (total - correct) / static_cast<double>(total)
		     << endl;
	}

	if (line == "TEST") {
		correct = 0;
		total = 0;
		while (getline(cin, line)) {
			correct += predict_line(line, m, cout);
			++total;
		}
		cerr << "Test error rate = "
		     << (total - correct) << " / " << total
		     << " = " << (total - correct) / static_cast<double>(total)
		     << endl;
	}

	return 0;
}
