#include "data.hh"
#include "maxent.hh"

#include <iostream>
#include <string>

using namespace std;

int main(int argc, char *argv[])
{
	if (argc != 2) {
		cerr << "Usage: " << argv[0] << " lambda < data > weights" << endl;
		return 1;
	}

	double lambda = stod(argv[1]);

	DataSet data(cin);

	cerr << "Data loaded:"
	     << " train " << data.train.size()
	     << " dev " << data.dev.size()
	     << " test " << data.test.size()
	     << endl;

	MaxEntModel model;

	model.train(data, lambda);

	model.dump_model(cout);

	return 0;
}
