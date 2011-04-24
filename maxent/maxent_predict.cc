#include "maxent.hh"

#include <fstream>
#include <iostream>
#include <sstream>
#include <ios>
#include <iomanip>

using namespace std;

int main(int argc, char *argv[])
{
	if (argc != 2) {
		cerr << "Usage: " << argv[0] << " weights < input > output" << endl;
		return 1;
	}

	ifstream fin(argv[1]);
	MaxEntModel m;
	m.load_model(fin);

	cerr << "Model loaded: "
	     << "path = " << argv[1] << " "
	     << "label_count = " << m.label_count << " "
	     << "feat_count = " << m.feat_count << endl;

	string line;
	size_t correct = 0;
	size_t total = 0;
	while (getline(cin, line)) {
		istringstream strin(line);

		size_t ref_label;
		strin >> ref_label;

		sparse_t sparse = m.build_sparse(strin);

		vec_t probs;
		size_t pred_label = m.predict(sparse, probs);

		cout << pred_label;
		for (auto i = probs.begin(); i != probs.end(); ++i)
			cout << (i == probs.begin() ? '\t' : ' ')
			     << setiosflags(ios_base::fixed)
			     << setprecision(20)
			     << *i;
		cout << '\n';

		correct += (pred_label == ref_label);
		++total;
	}
	cerr << "Error rate = "
	     << (total - correct) << " / " << total
	     << " = " << (total - correct) / static_cast<double>(total)
	     << endl;
	return 0;
}
