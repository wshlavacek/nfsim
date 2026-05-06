#include "NFfunction.hh"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>



using namespace std;
using namespace NFcore;

namespace {

string tfun_to_lower(string s) {
	std::transform(s.begin(), s.end(), s.begin(),
		[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	return s;
}

void tfun_trim_in_place(string &s) {
	while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) {
		s.erase(s.begin());
	}
	while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) {
		s.pop_back();
	}
}

double tfun_interpolate_value(
	const vector<double> &xs,
	const vector<double> &ys,
	const string &method,
	double x)
{
	if (xs.size() < 2 || ys.size() != xs.size()) {
		throw std::runtime_error("TFUN interpolation requires at least 2 rows with matching x/y lengths.");
	}

	if (x <= xs.front()) return ys.front();
	if (x >= xs.back()) return ys.back();

	auto it = std::upper_bound(xs.begin(), xs.end(), x);
	int i = static_cast<int>(it - xs.begin()) - 1;
	if (i < 0) i = 0;
	if (i >= static_cast<int>(xs.size()) - 1) i = static_cast<int>(xs.size()) - 2;

	if (method == "step") {
		return ys[static_cast<size_t>(i)];
	}

	double x0 = xs[static_cast<size_t>(i)];
	double x1 = xs[static_cast<size_t>(i + 1)];
	double y0 = ys[static_cast<size_t>(i)];
	double y1 = ys[static_cast<size_t>(i + 1)];
	double frac = (x - x0) / (x1 - x0);
	return y0 + frac * (y1 - y0);
}

}  // namespace


GlobalFunction::GlobalFunction(string name,
		string funcExpression,
		vector <string> &varRefNames,
		vector <string> &varRefTypes,
		vector <string> &paramNames,
		System *s)
{
	if(varRefNames.size()!=varRefTypes.size()) {
		cerr<<"Trying to create a global function, but your variable reference vectors don't match up in size!"<<endl;
		cerr<<"Quitting!"<<endl;
  throw std::runtime_error("Quitting!");
	}

	this->name = name;
	this->funcExpression = funcExpression;

	this->n_varRefs=varRefNames.size();
	this->varRefNames = new string[n_varRefs];
	this->varRefTypes = new string[n_varRefs];
	for(unsigned int vr=0; vr<n_varRefs; vr++) {
		this->varRefNames[vr]=varRefNames.at(vr);
		this->varRefTypes[vr]=varRefTypes.at(vr);
	}

	this->n_params=paramNames.size();
	this->paramNames = new string[n_params];
	for(unsigned int i=0; i<n_params; i++) {
		this->paramNames[i]=paramNames.at(i);
	}
	p=0;

	// AS-2021
	this->fileFunc = false;
	this->sysPtr = NULL;
	this->counter = NULL;
	this->currInd = 0;
	this->dataLen = 0;
	this->interpolationMethod = "linear";
	// AS-2021
}



GlobalFunction::~GlobalFunction()
{
	delete [] varRefNames;
	delete [] varRefTypes;
	delete [] paramNames;
	if(p!=NULL) delete p;
}




void GlobalFunction::prepareForSimulation(System *s)
{
	try {
		p=FuncFactory::create();
		for(unsigned int vr=0; vr<n_varRefs; vr++)
		{
			if(varRefTypes[vr]=="Observable") {
				Observable *obs = s->getObservableByName(varRefNames[vr]);
				if(obs==NULL) {
					cout<<"When creating global function: "<<this->name<<endl<<" could not find the observable: ";
					cout<<varRefNames[vr]<<" of type "<<varRefTypes[vr]<<endl;
					cout<<"Quitting."<<endl;
     throw std::runtime_error("Quitting");
				}
				obs->addReferenceToMyself(p);
			} else {
				cout<<"here"<<endl;
				cout<<"Uh oh, an unrecognized argType ("<<varRefTypes[vr]<<") for a function! "<<varRefNames[vr]<<endl;
				cout<<"Try using the type: \"MoleculeObservable\""<<endl;
				cout<<"Quitting because this will give unpredicatable results, or just crash."<<endl;
    throw std::runtime_error("Quitting because this will give unpredicatable results, or just crash");
			}
		}

		for(unsigned int i=0; i<n_params; i++) {
			p->DefineConst(paramNames[i],s->getParameter(paramNames[i]));
		}

		// TFUN placeholder must exist at compile time; fileUpdate() overwrites it.
		if (this->fileFunc && !this->ctrName.empty()) {
			p->DefineConst(this->ctrName, 0.0);
		}
		p->SetExpr(this->funcExpression);

	}
	catch (mu::Parser::exception_type &e)
	{
		cout<<"Error preparing function "<<name<<" in class GlobalFunction!!  This is what happened:"<<endl;
		cout<< "  "<<e.GetMsg() << endl;
		cout<<"Quitting."<<endl;
  throw std::runtime_error("Quitting");
	}
}

void GlobalFunction::updateParameters(System *s) {
	//cout<<"Updating parameters for function: "<<name<<endl;
	for(unsigned int i=0; i<n_params; i++) {
		p->DefineConst(paramNames[i],s->getParameter(paramNames[i]));
	}

}




void GlobalFunction::attatchRxn(ReactionClass *r)
{
	//unsigned int n_rxns;
	//ReactionClass *rxns;

}





void GlobalFunction::printDetails()
{
	cout<<"Global Function: '"<< this->name << "()'"<<endl;
	cout<<" ="<<funcExpression<<endl;
	cout<<"   -Variable References:"<<endl;
	for(unsigned int vr=0; vr<n_varRefs; vr++) {
		cout<<"         "<<varRefTypes[vr]<<":  "<<varRefNames[vr]<<" = " << ""<<endl;
	}
	cout<<"   -Constant Parameters:"<<endl;
	for(unsigned int i=0; i<n_params; i++) {
		cout<<"         "<<paramNames[i]<<endl;
	}





//	// Get the map with the variables
//	mu::Parser::varmap_type variables = p->GetVar();
//	cout << (int)variables.size() << " variables."<<endl;
//	mu::Parser::varmap_type::const_iterator item = variables.begin();
//	// Query the variables
//	for (; item!=variables.end(); ++item)
//	{
//	  cout << "  Name: " << item->first << " Address: [0x" << item->second << "]  Value: "<< *(item->second)<<"\n";
//	}


	if(p!=0) {
		// AS-2021
		if (this->fileFunc==true) {
			this->fileUpdate();
		}
		// AS-2021
		cout<<"   Function currently evaluates to: "<<FuncFactory::Eval(p)<<endl;
	}
}

// AS-2021
void GlobalFunction::loadParamFile(string filePath) 
{
	vector<double> xs;
	vector<double> ys;
	ifstream file(filePath.c_str());
	if (!file.good()) {
		cout<<"Error preparing function "<<this->name<<" in class GlobalFunction!!"<<endl;
		cout<<"File doesn't look like it exists"<<endl;
		cout<<"Quitting."<<endl;
		throw std::runtime_error("Quitting");
	}

	try {
		string line;
		while (std::getline(file, line)) {
			size_t comment = line.find('#');
			if (comment != string::npos) line = line.substr(0, comment);
			tfun_trim_in_place(line);
			if (line.empty()) continue;

			for (char &c : line) {
				if (c == ',') c = ' ';
			}

			std::istringstream iss(line);
			double x = 0.0;
			double y = 0.0;
			if (!(iss >> x >> y)) {
				cout<<"Error preparing function "<<this->name<<" in class GlobalFunction!!"<<endl;
				cout<<"Failed to parse TFUN data line: '"<<line<<"'"<<endl;
				cout<<"Quitting."<<endl;
				throw std::runtime_error("Quitting");
			}
			string trailing;
			if (iss >> trailing) {
				cout<<"Error preparing function "<<this->name<<" in class GlobalFunction!!"<<endl;
				cout<<"Unexpected trailing token in TFUN line: '"<<line<<"'"<<endl;
				cout<<"Quitting."<<endl;
				throw std::runtime_error("Quitting");
			}
			xs.push_back(x);
			ys.push_back(y);
		}
	} catch (exception const &e) {
		cout<<"Error preparing function "<<this->name<<" in class GlobalFunction!!"<<endl;
		cout<<"Failed to either open or read the file."<<endl;
		cout<<"Quitting."<<endl;
		throw std::runtime_error("Quitting");
	}

	if (xs.size() < 2 || ys.size() != xs.size()) {
		cout<<"Error preparing function "<<this->name<<" in class GlobalFunction!!"<<endl;
		cout<<"TFUN file must contain at least two data rows with equal x/y lengths."<<endl;
		cout<<"Quitting."<<endl;
		throw std::runtime_error("Quitting");
	}
	for (size_t i = 1; i < xs.size(); ++i) {
		if (xs[i] <= xs[i - 1]) {
			cout<<"Error preparing function "<<this->name<<" in class GlobalFunction!!"<<endl;
			cout<<"TFUN xData must be strictly increasing."<<endl;
			cout<<"Quitting."<<endl;
			throw std::runtime_error("Quitting");
		}
	}

	this->data.clear();
	this->data.push_back(xs);
	this->data.push_back(ys);
};

void GlobalFunction::addCounterPointer(double *counter){
	this->ctrType = "Observable";
	this->counter = counter;
}

void GlobalFunction::setCounterFromTime(System *s) {
	this->ctrType = "Time";
	this->sysPtr = s;
}

void GlobalFunction::setCounterFromParameter(System *s, string paramName) {
	this->ctrType = "Parameter";
	this->sysPtr = s;
	this->counterParamName = paramName;
}

void GlobalFunction::setCtrName(string name) {
	this->ctrName = name;
}

void GlobalFunction::setInterpolationMethod(string method) {
	string normalized = tfun_to_lower(method);
	if (normalized.empty()) normalized = "linear";
	if (normalized != "linear" && normalized != "step") {
		throw std::runtime_error("Unsupported TFUN interpolation method.");
	}
	this->interpolationMethod = normalized;
}

// unhooking system timer option for now
// void GlobalFunction::addSystemPointer(System *s) {
// 	this->ctrType = "System";
// 	this->sysPtr = s;
// }

void GlobalFunction::enableFileDependency(string filePath, string method) {
	// load file
	// TODO: Err out if this fails
	try {
		this->loadParamFile(filePath);
	} catch (exception const & e) {
		cout<<"Error preparing function "<<name<<" in class GlobalFunction!!"<<endl;
		cout<<"Quitting."<<endl;
  throw std::runtime_error("Quitting");
	};
	// we just want to keep a record of this
	this->filePath = filePath;
	// this sets it up so that this function knows it's supposed
	// to be pulling values from a file
	this->fileFunc = true;
	this->setInterpolationMethod(method);
	// initialize internal index
	this->currInd = 0;
	// pull data lenght so we can reuse it
	this->dataLen = static_cast<int>(data[0].size());
}

void GlobalFunction::enableInlineDependency(
	const vector<double> &xs,
	const vector<double> &ys,
	string method)
{
	if (xs.size() < 2 || ys.size() != xs.size()) {
		throw std::runtime_error("Inline TFUN data must have equal x/y lengths with at least 2 rows.");
	}
	for (size_t i = 1; i < xs.size(); ++i) {
		if (xs[i] <= xs[i - 1]) {
			throw std::runtime_error("Inline TFUN xData must be strictly increasing.");
		}
	}
	this->data.clear();
	this->data.push_back(xs);
	this->data.push_back(ys);
	this->filePath = "<inline>";
	this->fileFunc = true;
	this->setInterpolationMethod(method);
	this->currInd = 0;
	this->dataLen = static_cast<int>(xs.size());
}

double GlobalFunction::getCounterValue() {
	double ctrVal = 0.0;
	if (ctrType == "Observable") {
		if (counter == NULL) {
			throw std::runtime_error("GlobalFunction TFUN observable counter pointer is null.");
		}
		ctrVal = (*counter);
	} else if (ctrType == "Time") {
		if (sysPtr == NULL) {
			throw std::runtime_error("GlobalFunction TFUN time counter system pointer is null.");
		}
		ctrVal = this->sysPtr->getCurrentTime();
	} else if (ctrType == "Parameter") {
		if (sysPtr == NULL || counterParamName.empty()) {
			throw std::runtime_error("GlobalFunction TFUN parameter counter is not configured.");
		}
		ctrVal = this->sysPtr->getParameter(counterParamName);
	} else {
		throw std::runtime_error("GlobalFunction TFUN counter type is not configured.");
	}
	return ctrVal;
}
void GlobalFunction::fileUpdate() {
	double ctrVal = this->getCounterValue();
	double y = tfun_interpolate_value(data[0], data[1], interpolationMethod, ctrVal);
	p->DefineConst(ctrName, y);
	return;
}
// AS-2021

void GlobalFunction::printDetails(System *s)
{
	cout<<"Global Function: '"<< this->name << "()'"<<endl;
	cout<<" ="<<funcExpression<<endl;
	cout<<"   -Variable References:"<<endl;
	for(unsigned int vr=0; vr<n_varRefs; vr++) {
		cout<<"         "<<varRefTypes[vr]<<":  "<<varRefNames[vr]<<" = " << s->getObservableByName(varRefNames[vr])->getCount()<<endl;
	}
	cout<<"   -Constant Parameters:"<<endl;
	for(unsigned int i=0; i<n_params; i++) {
		cout<<"         "<<paramNames[i]<<" = " << s->getParameter(paramNames[i])<<endl;
	}

	
	if(p!=0) {
		// AS-2021
		if (this->fileFunc==true) {
			cout<<"   Function relies on file: "<<this->filePath<<endl;
			this->fileUpdate();
		}
		// AS-2021
		cout<<"   Function currently evaluates to: "<<FuncFactory::Eval(p)<<endl;
	}
}





StateCounter::StateCounter(string name, MoleculeType *mt, string stateName) {
	this->name=name;
	this->mt = mt;
	this->stateIndex = mt->getCompIndexFromName(stateName);
	this->value=0;

	//Make sure this is a state we can count on!
	if(!mt->isIntegerComponent(stateName)) {
		//if it is not an integer component state, we must abort because
		//we can not evaluate the sum of a non integer component
		cerr<<"Trying to create a stateCounter: '"<<name<<"' on the state: '"<<stateName<<"'\n";
		cerr<<"of MoleculeType: '"<<mt->getName()<<"', but the state you have selected cannot\n";
		cerr<<"have integer values, so summations on this state are undefined.  I am quitting."<<endl;
  throw std::runtime_error("have integer values, so summations on this state are undefined.  I am quitting");
	}
}
StateCounter::~StateCounter() {
	mt=0;
}

void StateCounter::add(Molecule *m) {
	if(m->getMoleculeType()==mt) {

	//	cout<<"matched moleculeType"<<endl;
		value+=m->getComponentState(stateIndex);
	//	cout<<"found component state: "<<m->getComponentState(stateIndex)<<endl;
	//	cout<<"updating v`alue to: "<< value<<endl;
	}
}
