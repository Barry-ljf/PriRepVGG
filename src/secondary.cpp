
#include "connect.h" 
#include "secondary.h"
#include <iostream>

extern CommunicationObject commObject;
extern int partyNum;
extern string * addrs;
extern BmrNet ** communicationSenders;
extern BmrNet ** communicationReceivers;
extern void log_print(string str);
#define NANOSECONDS_PER_SEC 1E9

//For time measurements
clock_t tStart;
struct timespec requestStart, requestEnd;
bool alreadyMeasuringTime = false;
int roundComplexitySend = 0;
int roundComplexityRecv = 0;
bool alreadyMeasuringRounds = false;

//For faster modular operations
extern smallType additionModPrime[PRIME_NUMBER][PRIME_NUMBER];
extern smallType subtractModPrime[PRIME_NUMBER][PRIME_NUMBER];
extern smallType multiplicationModPrime[PRIME_NUMBER][PRIME_NUMBER];

RSSVectorMyType trainData, testData;
RSSVectorMyType trainLabels, testLabels;
size_t trainDataBatchCounter = 0;
size_t trainLabelsBatchCounter = 0;
size_t testDataBatchCounter = 0;
size_t testLabelsBatchCounter = 0;

size_t INPUT_SIZE;
size_t LAST_LAYER_SIZE;
size_t NUM_LAYERS;
bool WITH_NORMALIZATION;
bool LARGE_NETWORK;
size_t TRAINING_DATA_SIZE;
size_t TEST_DATA_SIZE;
string SECURITY_TYPE;

extern void print_linear(myType var, string type);
extern void funcReconstruct(const RSSVectorMyType &a, vector<myType> &b, size_t size, string str, bool print);

/******************* Main train and test functions *******************/
void parseInputs(int argc, char* argv[])
{	
	if (argc < 6) 
		print_usage(argv[0]);

	partyNum = atoi(argv[1]);

	for (int i = 0; i < PRIME_NUMBER; ++i)
		for (int j = 0; j < PRIME_NUMBER; ++j)
		{
			additionModPrime[i][j] = ((i + j) % PRIME_NUMBER);
			subtractModPrime[i][j] = ((PRIME_NUMBER + i - j) % PRIME_NUMBER);
			multiplicationModPrime[i][j] = ((i * j) % PRIME_NUMBER); //How come you give the right answer multiplying in 8-bits??
		}
}

void train(NeuralNetwork* net)
{
	log_print("train");
	vector<size_t> counter(2,0);
	RSSVectorMyType maxIndex(MINI_BATCH_SIZE);
	

	for (int i = 0; i < NUM_ITERATIONS; ++i)
	{
		// cout << "----------------------------------" << endl;  
		// cout << "Iteration " << i << endl;
		readMiniBatch(net, "TRAINING");
		net->forward();
		net->backward();
		net->getAccuracy(maxIndex, counter);
		// cout << "----------------------------------" << endl;  
	}
}


extern void print_vector(RSSVectorMyType &var, string type, string pre_text, int print_nos);
extern string which_network(string network);
void test(bool PRELOADING, string network, NeuralNetwork* net)
{
	log_print("test");
	cout<<"code run secondary.o 88 "<<endl;
	//counter[0]: Correct samples, counter[1]: total samples
	//vector<size_t> counter(2,0);
	//RSSVectorMyType maxIndex(MINI_BATCH_SIZE);

	for (int i = 0; i < NUM_ITERATIONS; ++i)
	{
		if (!PRELOADING)
			readMiniBatch(net, "TESTING");

		net->forward();
		cout<<"code run secondary.o 99 net->forward();"<<endl;
		// net->predict(maxIndex);
		//net->getAccuracy(maxIndex, counter);

	}
	//print_vector((*(net->layers[NUM_LAYERS-1])->getActivation()), "FLOAT", "MPC Output over uint32_t:", 1280);

	// Write output to file
	if (PRELOADING)
	{
		ofstream data_file;
		data_file.open("files/preload/"+which_network(network)+"/"+which_network(network)+".txt");
		
		vector<myType> b(MINI_BATCH_SIZE * LAST_LAYER_SIZE);
		funcReconstruct((*(net->layers[NUM_LAYERS-1])->getActivation()), b, MINI_BATCH_SIZE * LAST_LAYER_SIZE, "anything", false);
		for (int i = 0; i < MINI_BATCH_SIZE; ++i)
		{
			for (int j = 0; j < LAST_LAYER_SIZE; ++j)
				data_file << b[i*(LAST_LAYER_SIZE) + j] << " ";
			data_file << endl;
		}
	}
}


// Generate a file with 0's of appropriate size
void generate_zeros(string name, size_t number, string network)
{
	string default_path = "files/preload/"+which_network(network)+"/";
	ofstream data_file;
	data_file.open(default_path+name);

	for (int i = 0; i < number; ++i)
		data_file << (int)0 << " ";
}


extern size_t nextParty(size_t party);
#include "FCLayer.h"
#include "CNNLayer.h"
#include "BNLayer.h"
#include "AdaptAvgpoolLayer.h"
void preload_network(bool PRELOADING, string network, NeuralNetwork* net)
{
	log_print("preload_network");
	//assert((PRELOADING) and (NUM_ITERATIONS == 1) and (MINI_BATCH_SIZE == 128) && "Preloading conditions fail");

	float temp_next = 0, temp_prev = 0;
	string default_path = "files/preload/"+which_network(network)+"/";
	//Set to true if you want the zeros files generated.
	bool ZEROS = true;

	if (which_network(network).compare("SecureML") == 0)
	{
		string temp = "SecureML";
		/************************** Input **********************************/
		string path_input_1 = default_path+"input_"+to_string(partyNum);
		string path_input_2 = default_path+"input_"+to_string(nextParty(partyNum));
		ifstream f_input_1(path_input_1), f_input_2(path_input_2);

		for (int i = 0; i < INPUT_SIZE * MINI_BATCH_SIZE; ++i)
		{
			f_input_1 >> temp_next; f_input_2 >> temp_prev;
			net->inputData[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_input_1.close(); f_input_2.close();
		if (ZEROS)
		{
			generate_zeros("input_1", 784*128, temp);
			generate_zeros("input_2", 784*128, temp);
		}

		// print_vector(net->inputData, "FLOAT", "inputData:", 784);

		/************************** Weight1 **********************************/
		string path_weight1_1 = default_path+"weight1_"+to_string(partyNum);
		string path_weight1_2 = default_path+"weight1_"+to_string(nextParty(partyNum));
		ifstream f_weight1_1(path_weight1_1), f_weight1_2(path_weight1_2);

		for (int column = 0; column < 128; ++column)
		{
			for (int row = 0; row < 784; ++row)
			{
				f_weight1_1 >> temp_next; f_weight1_2 >> temp_prev;
				(*((FCLayer*)net->layers[0])->getWeights())[128*row + column] = 
						std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
			}
		}
		f_weight1_1.close(); f_weight1_2.close();
		if (ZEROS)
		{
			generate_zeros("weight1_1", 784*128, temp);
			generate_zeros("weight1_2", 784*128, temp);
		}

		/************************** Weight2 **********************************/
		string path_weight2_1 = default_path+"weight2_"+to_string(partyNum);
		string path_weight2_2 = default_path+"weight2_"+to_string(nextParty(partyNum));
		ifstream f_weight2_1(path_weight2_1), f_weight2_2(path_weight2_2);

		for (int column = 0; column < 128; ++column)
		{
			for (int row = 0; row < 128; ++row)
			{
				f_weight2_1 >> temp_next; f_weight2_2 >> temp_prev;
				(*((FCLayer*)net->layers[2])->getWeights())[128*row + column] = 
						std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
			}
		}
		f_weight2_1.close(); f_weight2_2.close();
		if (ZEROS)
		{
			generate_zeros("weight2_1", 128*128, temp);
			generate_zeros("weight2_2", 128*128, temp);
		}

		/************************** Weight3 **********************************/
		string path_weight3_1 = default_path+"weight3_"+to_string(partyNum);
		string path_weight3_2 = default_path+"weight3_"+to_string(nextParty(partyNum));
		ifstream f_weight3_1(path_weight3_1), f_weight3_2(path_weight3_2);

		for (int column = 0; column < 10; ++column)
		{
			for (int row = 0; row < 128; ++row)
			{
				f_weight3_1 >> temp_next; f_weight3_2 >> temp_prev;
				(*((FCLayer*)net->layers[4])->getWeights())[10*row + column] = 
						std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
			}
		}
		f_weight3_1.close(); f_weight3_2.close();
		if (ZEROS)
		{
			generate_zeros("weight3_1", 128*10, temp);
			generate_zeros("weight3_2", 128*10, temp);
		}


		/************************** Bias1 **********************************/
		string path_bias1_1 = default_path+"bias1_"+to_string(partyNum);
		string path_bias1_2 = default_path+"bias1_"+to_string(nextParty(partyNum));
		ifstream f_bias1_1(path_bias1_1), f_bias1_2(path_bias1_2);

		for (int i = 0; i < 128; ++i)
		{
			f_bias1_1 >> temp_next; f_bias1_2 >> temp_prev;
			(*((FCLayer*)net->layers[0])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias1_1.close(); f_bias1_2.close();
		if (ZEROS)
		{
			generate_zeros("bias1_1", 128, temp);
			generate_zeros("bias1_2", 128, temp);
		}


		/************************** Bias2 **********************************/
		string path_bias2_1 = default_path+"bias2_"+to_string(partyNum);
		string path_bias2_2 = default_path+"bias2_"+to_string(nextParty(partyNum));
		ifstream f_bias2_1(path_bias2_1), f_bias2_2(path_bias2_2);

		for (int i = 0; i < 128; ++i)
		{
			f_bias2_1 >> temp_next; f_bias2_2 >> temp_prev;
			(*((FCLayer*)net->layers[2])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias2_1.close(); f_bias2_2.close();
		if (ZEROS)
		{
			generate_zeros("bias2_1", 128, temp);
			generate_zeros("bias2_2", 128, temp);
		}


		/************************** Bias3 **********************************/
		string path_bias3_1 = default_path+"bias3_"+to_string(partyNum);
		string path_bias3_2 = default_path+"bias3_"+to_string(nextParty(partyNum));
		ifstream f_bias3_1(path_bias3_1), f_bias3_2(path_bias3_2);

		for (int i = 0; i < 10; ++i)
		{
			f_bias3_1 >> temp_next; f_bias3_2 >> temp_prev;
			(*((FCLayer*)net->layers[4])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias3_1.close(); f_bias3_2.close();
		if (ZEROS)
		{
			generate_zeros("bias3_1", 10, temp);
			generate_zeros("bias3_2", 10, temp);
		}
	}
	else if (which_network(network).compare("Sarda") == 0)
	{
		string temp = "Sarda";
		/************************** Input **********************************/
		string path_input_1 = default_path+"input_"+to_string(partyNum);
		string path_input_2 = default_path+"input_"+to_string(nextParty(partyNum));
		ifstream f_input_1(path_input_1), f_input_2(path_input_2);

		for (int i = 0; i < INPUT_SIZE * MINI_BATCH_SIZE; ++i)
		{
			f_input_1 >> temp_next; f_input_2 >> temp_prev;
			net->inputData[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_input_1.close(); f_input_2.close();
		if (ZEROS)
		{
			generate_zeros("input_1", 784*128, temp);
			generate_zeros("input_2", 784*128, temp);
		}

		// print_vector(net->inputData, "FLOAT", "inputData:", 784);

		/************************** Weight1 **********************************/
		string path_weight1_1 = default_path+"weight1_"+to_string(partyNum);
		string path_weight1_2 = default_path+"weight1_"+to_string(nextParty(partyNum));
		ifstream f_weight1_1(path_weight1_1), f_weight1_2(path_weight1_2);

		for (int column = 0; column < 5; ++column)
		{
			for (int row = 0; row < 4; ++row)
			{
				f_weight1_1 >> temp_next; f_weight1_2 >> temp_prev;
				(*((CNNLayer*)net->layers[0])->getWeights())[4*column + row] = 
						std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
			}
		}
		f_weight1_1.close(); f_weight1_2.close();
		if (ZEROS)
		{
			generate_zeros("weight1_1", 2*2*1*5, temp);
			generate_zeros("weight1_2", 2*2*1*5, temp);
		}

		/************************** Weight2 **********************************/
		string path_weight2_1 = default_path+"weight2_"+to_string(partyNum);
		string path_weight2_2 = default_path+"weight2_"+to_string(nextParty(partyNum));
		ifstream f_weight2_1(path_weight2_1), f_weight2_2(path_weight2_2);

		for (int column = 0; column < 100; ++column)
		{
			for (int row = 0; row < 980; ++row)
			{
				f_weight2_1 >> temp_next; f_weight2_2 >> temp_prev;
				(*((FCLayer*)net->layers[2])->getWeights())[100*row + column] = 
						std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
			}
		}
		f_weight2_1.close(); f_weight2_2.close();
		if (ZEROS)
		{
			generate_zeros("weight2_1", 980*100, temp);
			generate_zeros("weight2_2", 980*100, temp);
		}


		/************************** Weight3 **********************************/
		string path_weight3_1 = default_path+"weight3_"+to_string(partyNum);
		string path_weight3_2 = default_path+"weight3_"+to_string(nextParty(partyNum));
		ifstream f_weight3_1(path_weight3_1), f_weight3_2(path_weight3_2);

		for (int column = 0; column < 10; ++column)
		{
			for (int row = 0; row < 100; ++row)
			{
				f_weight3_1 >> temp_next; f_weight3_2 >> temp_prev;
				(*((FCLayer*)net->layers[4])->getWeights())[10*row + column] = 
						std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
			}
		}
		f_weight3_1.close(); f_weight3_2.close();
		if (ZEROS)
		{
			generate_zeros("weight3_1", 100*10, temp);
			generate_zeros("weight3_2", 100*10, temp);
		}

		/************************** Bias1 **********************************/
		string path_bias1_1 = default_path+"bias1_"+to_string(partyNum);
		string path_bias1_2 = default_path+"bias1_"+to_string(nextParty(partyNum));
		ifstream f_bias1_1(path_bias1_1), f_bias1_2(path_bias1_2);

		for (int i = 0; i < 5; ++i)
		{
			f_bias1_1 >> temp_next; f_bias1_2 >> temp_prev;
			(*((CNNLayer*)net->layers[0])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias1_1.close(); f_bias1_2.close();
		if (ZEROS)
		{
			generate_zeros("bias1_1", 5, temp);
			generate_zeros("bias1_2", 5, temp);
		}

		/************************** Bias2 **********************************/
		string path_bias2_1 = default_path+"bias2_"+to_string(partyNum);
		string path_bias2_2 = default_path+"bias2_"+to_string(nextParty(partyNum));
		ifstream f_bias2_1(path_bias2_1), f_bias2_2(path_bias2_2);

		for (int i = 0; i < 100; ++i)
		{
			f_bias2_1 >> temp_next; f_bias2_2 >> temp_prev;
			(*((FCLayer*)net->layers[2])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias2_1.close(); f_bias2_2.close();
		if (ZEROS)
		{
			generate_zeros("bias2_1", 100, temp);
			generate_zeros("bias2_2", 100, temp);
		}

		/************************** Bias3 **********************************/
		string path_bias3_1 = default_path+"bias3_"+to_string(partyNum);
		string path_bias3_2 = default_path+"bias3_"+to_string(nextParty(partyNum));
		ifstream f_bias3_1(path_bias3_1), f_bias3_2(path_bias3_2);

		for (int i = 0; i < 10; ++i)
		{
			f_bias3_1 >> temp_next; f_bias3_2 >> temp_prev;
			(*((FCLayer*)net->layers[4])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias3_1.close(); f_bias3_2.close();
		if (ZEROS)
		{
			generate_zeros("bias3_1", 10, temp);
			generate_zeros("bias3_2", 10, temp);
		}
	}
	else if (which_network(network).compare("MiniONN") == 0)
	{
		string temp = "MiniONN";
		/************************** Input **********************************/
		string path_input_1 = default_path+"input_"+to_string(partyNum);
		string path_input_2 = default_path+"input_"+to_string(nextParty(partyNum));
		ifstream f_input_1(path_input_1), f_input_2(path_input_2);

		for (int i = 0; i < INPUT_SIZE * MINI_BATCH_SIZE; ++i)
		{
			f_input_1 >> temp_next; f_input_2 >> temp_prev;
			net->inputData[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_input_1.close(); f_input_2.close();
		if (ZEROS)
		{
			generate_zeros("input_1", 784*128, temp);
			generate_zeros("input_2", 784*128, temp);
		}

		// print_vector(net->inputData, "FLOAT", "inputData:", 784);

		/************************** Weight1 **********************************/
		string path_weight1_1 = default_path+"weight1_"+to_string(partyNum);
		string path_weight1_2 = default_path+"weight1_"+to_string(nextParty(partyNum));
		ifstream f_weight1_1(path_weight1_1), f_weight1_2(path_weight1_2);

		for (int row = 0; row < 5*5*1*16; ++row)
		{
			f_weight1_1 >> temp_next; f_weight1_2 >> temp_prev;
			(*((CNNLayer*)net->layers[0])->getWeights())[row] = 
					std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_weight1_1.close(); f_weight1_2.close();
		if (ZEROS)
		{
			generate_zeros("weight1_1", 5*5*1*16, temp);
			generate_zeros("weight1_2", 5*5*1*16, temp);
		}

		/************************** Weight2 **********************************/
		string path_weight2_1 = default_path+"weight2_"+to_string(partyNum);
		string path_weight2_2 = default_path+"weight2_"+to_string(nextParty(partyNum));
		ifstream f_weight2_1(path_weight2_1), f_weight2_2(path_weight2_2);


		for (int row = 0; row < 25*16*16; ++row)
		{
			f_weight2_1 >> temp_next; f_weight2_2 >> temp_prev;
			(*((CNNLayer*)net->layers[3])->getWeights())[row] = 
					std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_weight2_1.close(); f_weight2_2.close();
		if (ZEROS)
		{
			generate_zeros("weight2_1", 5*5*16*16, temp);
			generate_zeros("weight2_2", 5*5*16*16, temp);
		}

		/************************** Weight3 **********************************/
		string path_weight3_1 = default_path+"weight3_"+to_string(partyNum);
		string path_weight3_2 = default_path+"weight3_"+to_string(nextParty(partyNum));
		ifstream f_weight3_1(path_weight3_1), f_weight3_2(path_weight3_2);

		for (int column = 0; column < 100; ++column)
		{
			for (int row = 0; row < 256; ++row)
			{
				f_weight3_1 >> temp_next; f_weight3_2 >> temp_prev;
				(*((FCLayer*)net->layers[6])->getWeights())[100*row + column] = 
						std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
			}
		}
		f_weight3_1.close(); f_weight3_2.close();
		if (ZEROS)
		{
			generate_zeros("weight3_1", 256*100, temp);
			generate_zeros("weight3_2", 256*100, temp);
		}


		/************************** Weight4 **********************************/
		string path_weight4_1 = default_path+"weight4_"+to_string(partyNum);
		string path_weight4_2 = default_path+"weight4_"+to_string(nextParty(partyNum));
		ifstream f_weight4_1(path_weight4_1), f_weight4_2(path_weight4_2);

		for (int column = 0; column < 10; ++column)
		{
			for (int row = 0; row < 100; ++row)
			{
				f_weight4_1 >> temp_next; f_weight4_2 >> temp_prev;
				(*((FCLayer*)net->layers[8])->getWeights())[10*row + column] = 
						std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
			}
		}
		f_weight4_1.close(); f_weight4_2.close();
		if (ZEROS)
		{
			generate_zeros("weight4_1", 100*10, temp);
			generate_zeros("weight4_2", 100*10, temp);
		}

		/************************** Bias1 **********************************/
		string path_bias1_1 = default_path+"bias1_"+to_string(partyNum);
		string path_bias1_2 = default_path+"bias1_"+to_string(nextParty(partyNum));
		ifstream f_bias1_1(path_bias1_1), f_bias1_2(path_bias1_2);

		for (int i = 0; i < 16; ++i)
		{
			f_bias1_1 >> temp_next; f_bias1_2 >> temp_prev;
			(*((CNNLayer*)net->layers[0])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias1_1.close(); f_bias1_2.close();
		if (ZEROS)
		{
			generate_zeros("bias1_1", 16, temp);
			generate_zeros("bias1_2", 16, temp);
		}

		/************************** Bias2 **********************************/
		string path_bias2_1 = default_path+"bias2_"+to_string(partyNum);
		string path_bias2_2 = default_path+"bias2_"+to_string(nextParty(partyNum));
		ifstream f_bias2_1(path_bias2_1), f_bias2_2(path_bias2_2);

		for (int i = 0; i < 16; ++i)
		{
			f_bias2_1 >> temp_next; f_bias2_2 >> temp_prev;
			(*((CNNLayer*)net->layers[3])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias2_1.close(); f_bias2_2.close();
		if (ZEROS)
		{
			generate_zeros("bias2_1", 16, temp);
			generate_zeros("bias2_2", 16, temp);
		}

		/************************** Bias3 **********************************/
		string path_bias3_1 = default_path+"bias3_"+to_string(partyNum);
		string path_bias3_2 = default_path+"bias3_"+to_string(nextParty(partyNum));
		ifstream f_bias3_1(path_bias3_1), f_bias3_2(path_bias3_2);

		for (int i = 0; i < 100; ++i)
		{
			f_bias3_1 >> temp_next; f_bias3_2 >> temp_prev;
			(*((FCLayer*)net->layers[6])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias3_1.close(); f_bias3_2.close();
		if (ZEROS)
		{
			generate_zeros("bias3_1", 100, temp);
			generate_zeros("bias3_2", 100, temp);
		}

		/************************** Bias4 **********************************/
		string path_bias4_1 = default_path+"bias4_"+to_string(partyNum);
		string path_bias4_2 = default_path+"bias4_"+to_string(nextParty(partyNum));
		ifstream f_bias4_1(path_bias4_1), f_bias4_2(path_bias4_2);

		for (int i = 0; i < 10; ++i)
		{
			f_bias4_1 >> temp_next; f_bias4_2 >> temp_prev;
			(*((FCLayer*)net->layers[8])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias4_1.close(); f_bias4_2.close();
		if (ZEROS)
		{
			generate_zeros("bias4_1", 10, temp);
			generate_zeros("bias4_2", 10, temp);
		}
	}
	else if (which_network(network).compare("LeNet") == 0)
	{
		string temp = "LeNet";
		/************************** Input **********************************/
		string path_input_1 = default_path+"input_"+to_string(partyNum);
		string path_input_2 = default_path+"input_"+to_string(nextParty(partyNum));
		ifstream f_input_1(path_input_1), f_input_2(path_input_2);

		for (int i = 0; i < INPUT_SIZE * MINI_BATCH_SIZE; ++i)
		{
			f_input_1 >> temp_next; f_input_2 >> temp_prev;
			net->inputData[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_input_1.close(); f_input_2.close();
		if (ZEROS)
		{
			generate_zeros("input_1", 784*128, temp);
			generate_zeros("input_2", 784*128, temp);
		}

		// print_vector(net->inputData, "FLOAT", "inputData:", 784);

		/************************** labels for test --ljf**********************************/
		string path_labels_1 = default_path+"train_labels_"+to_string(partyNum);
		string path_labels_2 = default_path+"train_labels_"+to_string(nextParty(partyNum));
		ifstream f_labels_1(path_labels_1), f_labels_2(path_labels_2);

		for (int i = 0; i < LAST_LAYER_SIZE * MINI_BATCH_SIZE; ++i)
		{
			f_labels_1 >> temp_next; f_labels_2 >> temp_prev;
			net->outputData[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_labels_1.close(); f_labels_2.close();
		if (ZEROS)
		{
			generate_zeros("train_labels_1", 10*128, temp);
			generate_zeros("train_labels_2", 10*128, temp);
		}

		print_vector(net->outputData, "FLOAT", "outputData1:", 1280);



		/************************** Weight1 **********************************/
		string path_weight1_1 = default_path+"weight1_"+to_string(partyNum);
		string path_weight1_2 = default_path+"weight1_"+to_string(nextParty(partyNum));
		ifstream f_weight1_1(path_weight1_1), f_weight1_2(path_weight1_2);

		for (int row = 0; row < 5*5*1*20; ++row)
		{
			f_weight1_1 >> temp_next; f_weight1_2 >> temp_prev;
			(*((CNNLayer*)net->layers[0])->getWeights())[row] = 
					std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_weight1_1.close(); f_weight1_2.close();
		if (ZEROS)
		{
			generate_zeros("weight1_1", 5*5*1*20, temp);
			generate_zeros("weight1_2", 5*5*1*20, temp);
		}

		/************************** Weight2 **********************************/
		string path_weight2_1 = default_path+"weight2_"+to_string(partyNum);
		string path_weight2_2 = default_path+"weight2_"+to_string(nextParty(partyNum));
		ifstream f_weight2_1(path_weight2_1), f_weight2_2(path_weight2_2);


		for (int row = 0; row < 25*20*50; ++row)
		{
			f_weight2_1 >> temp_next; f_weight2_2 >> temp_prev;
			(*((CNNLayer*)net->layers[3])->getWeights())[row] = 
					std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_weight2_1.close(); f_weight2_2.close();
		if (ZEROS)
		{
			generate_zeros("weight2_1", 5*5*20*50, temp);
			generate_zeros("weight2_2", 5*5*20*50, temp);
		}

		/************************** Weight3 **********************************/
		string path_weight3_1 = default_path+"weight3_"+to_string(partyNum);
		string path_weight3_2 = default_path+"weight3_"+to_string(nextParty(partyNum));
		ifstream f_weight3_1(path_weight3_1), f_weight3_2(path_weight3_2);

		for (int column = 0; column < 500; ++column)
		{
			for (int row = 0; row < 800; ++row)
			{
				f_weight3_1 >> temp_next; f_weight3_2 >> temp_prev;
				(*((FCLayer*)net->layers[6])->getWeights())[500*row + column] = 
						std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
			}
		}
		f_weight3_1.close(); f_weight3_2.close();
		if (ZEROS)
		{
			generate_zeros("weight3_1", 800*500, temp);
			generate_zeros("weight3_2", 800*500, temp);
		}


		/************************** Weight4 **********************************/
		string path_weight4_1 = default_path+"weight4_"+to_string(partyNum);
		string path_weight4_2 = default_path+"weight4_"+to_string(nextParty(partyNum));	
		ifstream f_weight4_1(path_weight4_1), f_weight4_2(path_weight4_2);

		for (int column = 0; column < 10; ++column)
		{
			for (int row = 0; row < 500; ++row)
			{
				f_weight4_1 >> temp_next; f_weight4_2 >> temp_prev;
				(*((FCLayer*)net->layers[8])->getWeights())[10*row + column] = 
						std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
			}
		}
		f_weight4_1.close(); f_weight4_2.close();
		if (ZEROS)
		{
			generate_zeros("weight4_1", 500*10, temp);
			generate_zeros("weight4_2", 500*10, temp);
		}

		/************************** Bias1 **********************************/
		string path_bias1_1 = default_path+"bias1_"+to_string(partyNum);
		string path_bias1_2 = default_path+"bias1_"+to_string(nextParty(partyNum));
		ifstream f_bias1_1(path_bias1_1), f_bias1_2(path_bias1_2);

		for (int i = 0; i < 20; ++i)
		{
			f_bias1_1 >> temp_next; f_bias1_2 >> temp_prev;
			(*((CNNLayer*)net->layers[0])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias1_1.close(); f_bias1_2.close();
		if (ZEROS)
		{
			generate_zeros("bias1_1", 20, temp);
			generate_zeros("bias1_2", 20, temp);
		}

		/************************** Bias2 **********************************/
		string path_bias2_1 = default_path+"bias2_"+to_string(partyNum);
		string path_bias2_2 = default_path+"bias2_"+to_string(nextParty(partyNum));
		ifstream f_bias2_1(path_bias2_1), f_bias2_2(path_bias2_2);

		for (int i = 0; i < 50; ++i)
		{
			f_bias2_1 >> temp_next; f_bias2_2 >> temp_prev;
			(*((CNNLayer*)net->layers[3])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias2_1.close(); f_bias2_2.close();
		if (ZEROS)
		{
			generate_zeros("bias2_1", 50, temp);
			generate_zeros("bias2_2", 50, temp);
		} 

		/************************** Bias3 **********************************/
		string path_bias3_1 = default_path+"bias3_"+to_string(partyNum);
		string path_bias3_2 = default_path+"bias3_"+to_string(nextParty(partyNum));
		ifstream f_bias3_1(path_bias3_1), f_bias3_2(path_bias3_2);

		for (int i = 0; i < 500; ++i)
		{
			f_bias3_1 >> temp_next; f_bias3_2 >> temp_prev;
			(*((FCLayer*)net->layers[6])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias3_1.close(); f_bias3_2.close();
		if (ZEROS)
		{
			generate_zeros("bias3_1", 500, temp);
			generate_zeros("bias3_2", 500, temp);
		}

		/************************** Bias4 **********************************/
		string path_bias4_1 = default_path+"bias4_"+to_string(partyNum);
		string path_bias4_2 = default_path+"bias4_"+to_string(nextParty(partyNum));
		ifstream f_bias4_1(path_bias4_1), f_bias4_2(path_bias4_2);

		for (int i = 0; i < 10; ++i)
		{
			f_bias4_1 >> temp_next; f_bias4_2 >> temp_prev;
			(*((FCLayer*)net->layers[8])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias4_1.close(); f_bias4_2.close();
		if (ZEROS)
		{
			generate_zeros("bias4_1", 10, temp);
			generate_zeros("bias4_2", 10, temp);
		}
	}
	else if (which_network(network).compare("AlexNet") == 0)//--ljf ALexnet
	{
		string temp = "AlexNet";
				/************************** Input **********************************/
		string path_input_1 = default_path+"input_"+to_string(partyNum);
		string path_input_2 = default_path+"input_"+to_string(nextParty(partyNum));
		ifstream f_input_1(path_input_1), f_input_2(path_input_2);

		for (int i = 0; i < INPUT_SIZE * MINI_BATCH_SIZE; ++i)
		{
			f_input_1 >> temp_next; f_input_2 >> temp_prev;
			net->inputData[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_input_1.close(); f_input_2.close();
		if (ZEROS)
		{
			generate_zeros("input_1", 227*227*3*128, temp);
			generate_zeros("input_2", 227*227*3*128, temp);
		}

		//we don't have this for now.
		// /************************** labels for test --ljf**********************************/
		// string path_labels_1 = default_path+"train_labels_"+to_string(partyNum);
		// string path_labels_2 = default_path+"train_labels_"+to_string(nextParty(partyNum));
		// ifstream f_labels_1(path_labels_1), f_labels_2(path_labels_2);

		// for (int i = 0; i < LAST_LAYER_SIZE * MINI_BATCH_SIZE; ++i)
		// {
		// 	f_labels_1 >> temp_next; f_labels_2 >> temp_prev;
		// 	net->outputData[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		// }
		// f_labels_1.close(); f_labels_2.close();
		// if (ZEROS)
		// {
		// 	generate_zeros("train_labels_1", 128, temp);
		// 	generate_zeros("train_labels_2", 128, temp);//不需要onehot-coding
		// }
		// print_vector(net->outputData, "FLOAT", "outputData1:", 128);
		
		/**************************CNN -  Weight1 **********************************/ 
		string path_weight1_1 = default_path+"weight1_"+to_string(partyNum);
		string path_weight1_2 = default_path+"weight1_"+to_string(nextParty(partyNum));
		ifstream f_weight1_1(path_weight1_1), f_weight1_2(path_weight1_2);

		for (int row = 0; row <11 * 11 * 3 * 96 ; ++row)
		{
			f_weight1_1 >> temp_next; f_weight1_2 >> temp_prev;
			(*((CNNLayer*)net->layers[0])->getWeights())[row] = 
					std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_weight1_1.close(); f_weight1_2.close();
		if (ZEROS)
		{
			generate_zeros("weight1_1",11 * 11 * 3 * 96, temp);
			generate_zeros("weight1_2",11 * 11 * 3 * 96, temp);
		}
		std::cout<<"code run here 841!"<<endl;
				/**************************BN - Weight2 **********************************/
		string path_weight2_1 = default_path+"weight2_"+to_string(partyNum);
		string path_weight2_2 = default_path+"weight2_"+to_string(nextParty(partyNum));
		ifstream f_weight2_1(path_weight2_1), f_weight2_2(path_weight2_2);
			for (int row = 0; row < 96; ++row)
			{
				f_weight2_1 >> temp_next; f_weight2_2 >> temp_prev;
				(*((BNLayer*)net->layers[3])->getWeights())[row] = //to do
						std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
			}
		f_weight2_1.close(); f_weight2_2.close();
		if (ZEROS)
		{
			generate_zeros("weight2_1", 96, temp);
			generate_zeros("weight2_2", 96, temp);
		}
		std::cout<<"code run here 858!"<<endl;
		/**************************CNN -   Weight3 **********************************/
		
		string path_weight3_1 = default_path+"weight3_"+to_string(partyNum);
		string path_weight3_2 = default_path+"weight3_"+to_string(nextParty(partyNum));
		ifstream f_weight3_1(path_weight3_1), f_weight3_2(path_weight3_2);

		for (int row = 0; row < 5 * 5 * 96 * 256; ++row)
		{
			f_weight3_1 >> temp_next; f_weight3_2 >> temp_prev;
			(*((CNNLayer*)net->layers[4])->getWeights())[row] = 
					std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_weight3_1.close(); f_weight3_2.close();
		if (ZEROS)
		{
			generate_zeros("weight3_1",  5 * 5 * 96 * 256, temp);
			generate_zeros("weight3_2", 5 * 5 * 96 * 256, temp);
		}
		std::cout<<"code run here 877!"<<endl;
		/**************************  BN - Weight4 **********************************/
		string path_weight4_1 = default_path+"weight4_"+to_string(partyNum);
		string path_weight4_2 = default_path+"weight4_"+to_string(nextParty(partyNum));
		ifstream f_weight4_1(path_weight4_1), f_weight4_2(path_weight4_2);

		for (int row = 0; row < 256; ++row)
		{
			f_weight4_1 >> temp_next; f_weight4_2 >> temp_prev;
			(*((BNLayer*)net->layers[7])->getWeights())[row] = 
					std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_weight4_1.close(); f_weight4_2.close();
		if (ZEROS)
		{
			generate_zeros("weight4_1", 256, temp);
			generate_zeros("weight4_2", 256, temp);
		}

		std::cout<<"code run here 896!"<<endl;
		/************************** CNN -  Weight5 **********************************/		
		string path_weight5_1 = default_path+"weight5_"+to_string(partyNum);
		string path_weight5_2 = default_path+"weight5_"+to_string(nextParty(partyNum));
		ifstream f_weight5_1(path_weight5_1), f_weight5_2(path_weight5_2);
		std::cout<<"code run here 904!"<<endl;
		for (int row = 0; row < 3 * 3 * 256 * 384; ++row){
			f_weight5_1 >> temp_next; f_weight5_2 >> temp_prev;
			(*((CNNLayer*)net->layers[8])->getWeights())[row] = 
					std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		std::cout<<"code run here 911!"<<endl;
		f_weight5_1.close(); f_weight5_2.close();
		if (ZEROS)
		{
			generate_zeros("weight5_1",3 * 3 * 256 * 384, temp);
			generate_zeros("weight5_2",3 * 3 * 256 * 384, temp);
		}
		std::cout<<"code run here 914!"<<endl;
		/**************************CNN -  Weight6 **********************************/
		string path_weight6_1 = default_path+"weight6_"+to_string(partyNum);
		string path_weight6_2 = default_path+"weight6_"+to_string(nextParty(partyNum));
		ifstream f_weight6_1(path_weight6_1), f_weight6_2(path_weight6_2);

		for (int row = 0; row < 3 * 3 * 384 * 384; ++row)
		{
			f_weight6_1 >> temp_next; f_weight6_2 >> temp_prev;
			(*((CNNLayer*)net->layers[10])->getWeights())[row] = 
					std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_weight6_1.close(); f_weight6_2.close();
		if (ZEROS)
		{
			generate_zeros("weight6_1", 3 * 3 * 384 * 384, temp);
			generate_zeros("weight6_2", 3 * 3 * 384 * 384, temp);
		}

		/************************** CNN -  Weight7 **********************************/
		string path_weight7_1 = default_path+"weight7_"+to_string(partyNum);
		string path_weight7_2 = default_path+"weight7_"+to_string(nextParty(partyNum));
		ifstream f_weight7_1(path_weight7_1), f_weight7_2(path_weight7_2);

		for (int row = 0; row < 3 * 3 * 256 * 384; ++row)
		{
			f_weight7_1 >> temp_next; f_weight7_2 >> temp_prev;
			(*((CNNLayer*)net->layers[12])->getWeights())[row] = 
					std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_weight7_1.close(); f_weight7_2.close();
		if (ZEROS)
		{
			generate_zeros("weight7_1",3 * 3 * 256 * 384, temp);
			generate_zeros("weight7_2", 3 * 3 * 256 * 384, temp);
		}


		/************************** FC -Weight8 **********************************/
		string path_weight8_1 = default_path+"weight8_"+to_string(partyNum);
		string path_weight8_2 = default_path+"weight8_"+to_string(nextParty(partyNum));
		ifstream f_weight8_1(path_weight8_1), f_weight8_2(path_weight8_2);

		for (int column = 0; column < 4096; ++column)
		{
			for (int row = 0; row < 6*6*256; ++row)
			{
				f_weight8_1 >> temp_next; f_weight8_2 >> temp_prev;
				(*((FCLayer*)net->layers[15])->getWeights())[ 4096*row + column ] = 
						std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
			}
		}
		f_weight8_1.close(); f_weight8_2.close();
		if (ZEROS)
		{
			generate_zeros("weight8_1", 6*6*256*4096, temp);
			generate_zeros("weight8_2", 6*6*256*4096, temp);
		}
		std::cout<<"code run here 972!"<<endl;
				/************************** FC - Weight9 **********************************/
		string path_weight9_1 = default_path+"weight9_"+to_string(partyNum);
		string path_weight9_2 = default_path+"weight9_"+to_string(nextParty(partyNum));
		ifstream f_weight9_1(path_weight9_1), f_weight9_2(path_weight9_2);

		for (int column = 0; column < 4096; ++column)
		{
			for (int row = 0; row < 4096; ++row)
			{
				f_weight9_1 >> temp_next; f_weight9_2 >> temp_prev;
				(*((FCLayer*)net->layers[17])->getWeights())[4096*row + column] = 
						std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
			}
		}
		f_weight9_1.close(); f_weight9_2.close();
		if (ZEROS)
		{
			generate_zeros("weight9_1", 4096 * 4096, temp);
			generate_zeros("weight9_2", 4096 * 4096, temp);
		}
		std::cout<<"code run here 988!"<<endl;
		/**************************FC - Weight10 **********************************/
		string path_weight10_1 = default_path+"weight10_"+to_string(partyNum);
		string path_weight10_2 = default_path+"weight10_"+to_string(nextParty(partyNum));
		ifstream f_weight10_1(path_weight10_1), f_weight10_2(path_weight10_2);

		for (int column = 0; column < 2; ++column)
		{
			for (int row = 0; row < 4096; ++row)
			{
				f_weight10_1 >> temp_next; f_weight10_2 >> temp_prev;
				(*((FCLayer*)net->layers[19])->getWeights())[2*row + column] = 
						std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
			}
		}
		f_weight10_1.close(); f_weight10_2.close();
		if (ZEROS)
		{
			generate_zeros("weight10_1", 4096*2, temp);
			generate_zeros("weight10_2", 4096*2, temp);
		}

		/**************************CNN - Bias1 **********************************/
		string path_bias1_1 = default_path+"bias1_"+to_string(partyNum);
		string path_bias1_2 = default_path+"bias1_"+to_string(nextParty(partyNum));
		ifstream f_bias1_1(path_bias1_1), f_bias1_2(path_bias1_2);

		for (int i = 0; i < 96; ++i)
		{
			f_bias1_1 >> temp_next; f_bias1_2 >> temp_prev;
			(*((CNNLayer*)net->layers[0])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias1_1.close(); f_bias1_2.close();
		if (ZEROS)
		{
			generate_zeros("bias1_1", 96, temp);
			generate_zeros("bias1_2", 96, temp);
		}

		/************************** BN - Bias2 **********************************/
		string path_bias2_1 = default_path+"bias2_"+to_string(partyNum);
		string path_bias2_2 = default_path+"bias2_"+to_string(nextParty(partyNum));
		ifstream f_bias2_1(path_bias2_1), f_bias2_2(path_bias2_2);

		for (int i = 0; i < 96; ++i)
		{
			f_bias2_1 >> temp_next; f_bias2_2 >> temp_prev;
			(*((BNLayer*)net->layers[3])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias2_1.close(); f_bias2_2.close();
		if (ZEROS)
		{
			generate_zeros("bias2_1", 96, temp);
			generate_zeros("bias2_2", 96, temp);
		} 

		/**************************CNN -  Bias3 **********************************/
		string path_bias3_1 = default_path+"bias3_"+to_string(partyNum);
		string path_bias3_2 = default_path+"bias3_"+to_string(nextParty(partyNum));
		ifstream f_bias3_1(path_bias3_1), f_bias3_2(path_bias3_2);

		for (int i = 0; i < 256; ++i)
		{
			f_bias3_1 >> temp_next; f_bias3_2 >> temp_prev;
			(*((CNNLayer*)net->layers[4])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias3_1.close(); f_bias3_2.close();
		if (ZEROS)
		{
			generate_zeros("bias3_1",256 , temp);
			generate_zeros("bias3_2", 256, temp);
		}
		std::cout<<"code run here 1068!"<<endl;
		/**************************BN -  Bias4 **********************************/
		string path_bias4_1 = default_path+"bias4_"+to_string(partyNum);
		string path_bias4_2 = default_path+"bias4_"+to_string(nextParty(partyNum));
		ifstream f_bias4_1(path_bias4_1), f_bias4_2(path_bias4_2);

		for (int i = 0; i < 256; ++i)
		{
			f_bias4_1 >> temp_next; f_bias4_2 >> temp_prev;
			(*((BNLayer*)net->layers[7])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias4_1.close(); f_bias4_2.close();
		if (ZEROS)
		{
			generate_zeros("bias4_1", 256, temp);
			generate_zeros("bias4_2", 256, temp);
		}
		std::cout<<"code run here 1085!"<<endl;
			/**************************	CNN -  Bias5 **********************************/
		string path_bias5_1 = default_path+"bias5_"+to_string(partyNum);
		string path_bias5_2 = default_path+"bias5_"+to_string(nextParty(partyNum));
		ifstream f_bias5_1(path_bias5_1), f_bias5_2(path_bias5_2);
		std::cout<<"code run here 1110!" <<endl;
		for (int i = 0; i < 384; ++i)
		{
			f_bias5_1 >> temp_next; f_bias5_2 >> temp_prev;
			(*((CNNLayer*)net->layers[8])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias5_1.close(); f_bias5_2.close();
		if (ZEROS)
		{
			generate_zeros("bias5_1", 384, temp);
			generate_zeros("bias5_2", 384, temp);
		} 
		std::cout<<"code run here 1102!"<<endl;
		/************************** CNN - Bias6 **********************************/
		string path_bias6_1 = default_path+"bias6_"+to_string(partyNum);
		string path_bias6_2 = default_path+"bias6_"+to_string(nextParty(partyNum));
		ifstream f_bias6_1(path_bias6_1), f_bias6_2(path_bias6_2);
		std::cout<<"code run here 1110!" <<endl;
		for (int i = 0; i < 384; ++i)
		{
			f_bias6_1 >> temp_next; f_bias6_2 >> temp_prev;
			(*((CNNLayer*)net->layers[10])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias6_1.close(); f_bias6_2.close();
		if (ZEROS)
		{
			generate_zeros("bias6_1", 384, temp);
			generate_zeros("bias6_2", 384, temp);
		} 

		/************************** CNN - Bias7 **********************************/
		string path_bias7_1 = default_path+"bias7_"+to_string(partyNum);
		string path_bias7_2 = default_path+"bias7_"+to_string(nextParty(partyNum));
		ifstream f_bias7_1(path_bias7_1), f_bias7_2(path_bias7_2);

		for (int i = 0; i < 256; ++i)
		{
			f_bias7_1 >> temp_next; f_bias7_2 >> temp_prev;
			(*((CNNLayer*)net->layers[12])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias7_1.close(); f_bias7_2.close();
		if (ZEROS)
		{
			generate_zeros("bias7_1", 256, temp);
			generate_zeros("bias7_2", 256, temp);
		}

		/************************** FC - Bias8 **********************************/
		string path_bias8_1 = default_path+"bias8_"+to_string(partyNum);
		string path_bias8_2 = default_path+"bias8_"+to_string(nextParty(partyNum));
		ifstream f_bias8_1(path_bias8_1), f_bias8_2(path_bias8_2);

		for (int i = 0; i < 4096; ++i)
		{
			f_bias8_1 >> temp_next; f_bias8_2 >> temp_prev;
			(*((FCLayer*)net->layers[15])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias8_1.close(); f_bias8_2.close();
		if (ZEROS)
		{
			generate_zeros("bias8_1", 4096, temp);
			generate_zeros("bias8_2", 4096, temp);
		}

			/**************************FC - Bias9 **********************************/
		string path_bias9_1 = default_path+"bias9_"+to_string(partyNum);
		string path_bias9_2 = default_path+"bias9_"+to_string(nextParty(partyNum));
		ifstream f_bias9_1(path_bias9_1), f_bias9_2(path_bias9_2);

		for (int i = 0; i < 4096; ++i)
		{
			f_bias9_1 >> temp_next; f_bias9_2 >> temp_prev;
			(*((FCLayer*)net->layers[17])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias9_1.close(); f_bias9_2.close();
		if (ZEROS)
		{
			generate_zeros("bias9_1", 4096, temp);
			generate_zeros("bias9_2", 4096, temp);
		}

		/**************************FC -  Bias10 **********************************/
		string path_bias10_1 = default_path+"bias10_"+to_string(partyNum);
		string path_bias10_2 = default_path+"bias10_"+to_string(nextParty(partyNum));
		ifstream f_bias10_1(path_bias10_1), f_bias10_2(path_bias10_2);

		for (int i = 0; i < 2; ++i)
		{
			f_bias10_1 >> temp_next; f_bias10_2 >> temp_prev;
			(*((FCLayer*)net->layers[19])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias10_1.close(); f_bias10_2.close();
		if (ZEROS)
		{
			generate_zeros("bias10_1", 2, temp);
			generate_zeros("bias10_2", 2, temp);
		} 

// 		/************************** Input **********************************/
// 		string path_input_1 = default_path+"input_"+to_string(partyNum);
// 		string path_input_2 = default_path+"input_"+to_string(nextParty(partyNum));
// 		ifstream f_input_1(path_input_1), f_input_2(path_input_2);

// 		for (int i = 0; i < INPUT_SIZE * MINI_BATCH_SIZE; ++i)
// 		{
// 			f_input_1 >> temp_next; f_input_2 >> temp_prev;
// 			net->inputData[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
// 		}
// 		f_input_1.close(); f_input_2.close();
// 		if (ZEROS)
// 		{
// 			generate_zeros("input_1", 65*65*3*32, temp);
// 			generate_zeros("input_2", 65*65*3*32, temp);
// 		}

// 		//we don't have this for now.
// 		// /************************** labels for test --ljf**********************************/
// 		// string path_labels_1 = default_path+"train_labels_"+to_string(partyNum);
// 		// string path_labels_2 = default_path+"train_labels_"+to_string(nextParty(partyNum));
// 		// ifstream f_labels_1(path_labels_1), f_labels_2(path_labels_2);

// 		// for (int i = 0; i < LAST_LAYER_SIZE * MINI_BATCH_SIZE; ++i)
// 		// {
// 		// 	f_labels_1 >> temp_next; f_labels_2 >> temp_prev;
// 		// 	net->outputData[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
// 		// }
// 		// f_labels_1.close(); f_labels_2.close();
// 		// if (ZEROS)
// 		// {
// 		// 	generate_zeros("train_labels_1", 128, temp);
// 		// 	generate_zeros("train_labels_2", 128, temp);//不需要onehot-coding
// 		// }
// 		// print_vector(net->outputData, "FLOAT", "outputData1:", 128);
		
// 		/**************************CNN -  Weight1 **********************************/ 
// 		string path_weight1_1 = default_path+"weight1_"+to_string(partyNum);
// 		string path_weight1_2 = default_path+"weight1_"+to_string(nextParty(partyNum));
// 		ifstream f_weight1_1(path_weight1_1), f_weight1_2(path_weight1_2);

// 		for (int row = 0; row <11 * 11 * 3 * 48 ; ++row)
// 		{
// 			f_weight1_1 >> temp_next; f_weight1_2 >> temp_prev;
// 			(*((CNNLayer*)net->layers[0])->getWeights())[row] = 
// 					std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
// 		}
// 		f_weight1_1.close(); f_weight1_2.close();
// 		if (ZEROS)
// 		{
// 			generate_zeros("weight1_1",11 * 11 * 3 * 48, temp);
// 			generate_zeros("weight1_2",11 * 11 * 3 * 48, temp);
// 		}
// 		std::cout<<"code run here 841!"<<endl;
// 				/**************************BN - Weight2 **********************************/
// 		string path_weight2_1 = default_path+"weight2_"+to_string(partyNum);
// 		string path_weight2_2 = default_path+"weight2_"+to_string(nextParty(partyNum));
// 		ifstream f_weight2_1(path_weight2_1), f_weight2_2(path_weight2_2);
// 			for (int row = 0; row <48; ++row)
// 			{
// 				f_weight2_1 >> temp_next; f_weight2_2 >> temp_prev;
// 				(*((BNLayer*)net->layers[3])->getWeights())[row] = //to do
// 						std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
// 			}
// 		f_weight2_1.close(); f_weight2_2.close();
// 		if (ZEROS)
// 		{
// 			generate_zeros("weight2_1", 48, temp);
// 			generate_zeros("weight2_2", 48, temp);
// 		}
// 		std::cout<<"code run here 858!"<<endl;
// 		/**************************CNN -   Weight3 **********************************/
		
// 		string path_weight3_1 = default_path+"weight3_"+to_string(partyNum);
// 		string path_weight3_2 = default_path+"weight3_"+to_string(nextParty(partyNum));
// 		ifstream f_weight3_1(path_weight3_1), f_weight3_2(path_weight3_2);

// 		for (int row = 0; row < 5 * 5 * 128 * 48; ++row)
// 		{
// 			f_weight3_1 >> temp_next; f_weight3_2 >> temp_prev;
// 			(*((CNNLayer*)net->layers[4])->getWeights())[row] = 
// 					std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
// 		}
// 		f_weight3_1.close(); f_weight3_2.close();
// 		if (ZEROS)
// 		{
// 			generate_zeros("weight3_1", 5 * 5 * 48 * 128, temp);
// 			generate_zeros("weight3_2", 5 * 5 * 48 * 128, temp);
// 		}
// 		std::cout<<"code run here 877!"<<endl;
// 		/**************************  BN - Weight4 **********************************/
// 		string path_weight4_1 = default_path+"weight4_"+to_string(partyNum);
// 		string path_weight4_2 = default_path+"weight4_"+to_string(nextParty(partyNum));
// 		ifstream f_weight4_1(path_weight4_1), f_weight4_2(path_weight4_2);

// 		for (int row = 0; row < 128; ++row)
// 		{
// 			f_weight4_1 >> temp_next; f_weight4_2 >> temp_prev;
// 			(*((BNLayer*)net->layers[7])->getWeights())[row] = 
// 					std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
// 		}
// 		f_weight4_1.close(); f_weight4_2.close();
// 		if (ZEROS)
// 		{
// 			generate_zeros("weight4_1", 128, temp);
// 			generate_zeros("weight4_2", 128, temp);
// 		}

// 		std::cout<<"code run here 896!"<<endl;
// 		/************************** CNN -  Weight5 **********************************/		
// 		string path_weight5_1 = default_path+"weight5_"+to_string(partyNum);
// 		string path_weight5_2 = default_path+"weight5_"+to_string(nextParty(partyNum));
// 		ifstream f_weight5_1(path_weight5_1), f_weight5_2(path_weight5_2);
// 		std::cout<<"code run here 904!"<<endl;
// 		for (int row = 0; row < 3 * 3 * 128 * 192; ++row){
// 			f_weight5_1 >> temp_next; f_weight5_2 >> temp_prev;
// 			(*((CNNLayer*)net->layers[8])->getWeights())[row] = 
// 					std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
// 		}
// 		std::cout<<"code run here 911!"<<endl;
// 		f_weight5_1.close(); f_weight5_2.close();
// 		if (ZEROS)
// 		{
// 			generate_zeros("weight5_1",3 * 3 * 128 * 192, temp);
// 			generate_zeros("weight5_2",3 * 3 * 128 * 192, temp);
// 		}
// 		std::cout<<"code run here 914!"<<endl;
// 		/**************************CNN -  Weight6 **********************************/
// 		string path_weight6_1 = default_path+"weight6_"+to_string(partyNum);
// 		string path_weight6_2 = default_path+"weight6_"+to_string(nextParty(partyNum));
// 		ifstream f_weight6_1(path_weight6_1), f_weight6_2(path_weight6_2);

// 		for (int row = 0; row < 3 * 3 * 192 * 192; ++row)
// 		{
// 			f_weight6_1 >> temp_next; f_weight6_2 >> temp_prev;
// 			(*((CNNLayer*)net->layers[10])->getWeights())[row] = 
// 					std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
// 		}
// 		f_weight6_1.close(); f_weight6_2.close();
// 		if (ZEROS)
// 		{
// 			generate_zeros("weight6_1", 3 * 3 * 192 * 192, temp);
// 			generate_zeros("weight6_2", 3 * 3 * 192 * 192, temp);
// 		}

// 		/************************** CNN -  Weight7 **********************************/
// 		string path_weight7_1 = default_path+"weight7_"+to_string(partyNum);
// 		string path_weight7_2 = default_path+"weight7_"+to_string(nextParty(partyNum));
// 		ifstream f_weight7_1(path_weight7_1), f_weight7_2(path_weight7_2);

// 		for (int row = 0; row < 3 * 3 * 128 * 192; ++row)
// 		{
// 			f_weight7_1 >> temp_next; f_weight7_2 >> temp_prev;
// 			(*((CNNLayer*)net->layers[12])->getWeights())[row] = 
// 					std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
// 		}
// 		f_weight7_1.close(); f_weight7_2.close();
// 		if (ZEROS)
// 		{
// 			generate_zeros("weight7_1",3 * 3 * 128 * 192, temp);
// 			generate_zeros("weight7_2", 3 * 3 * 128 * 192, temp);
// 		}


// 		/************************** FC -Weight8 **********************************/
// 		string path_weight8_1 = default_path+"weight8_"+to_string(partyNum);
// 		string path_weight8_2 = default_path+"weight8_"+to_string(nextParty(partyNum));
// 		ifstream f_weight8_1(path_weight8_1), f_weight8_2(path_weight8_2);

// 		for (int column = 0; column < 1024; ++column)
// 		{
// 			for (int row = 0; row < 1152; ++row)
// 			{
// 				f_weight8_1 >> temp_next; f_weight8_2 >> temp_prev;
// 				(*((FCLayer*)net->layers[15])->getWeights())[ 1024*row + column ] = 
// 						std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
// 			}
// 		}
// 		f_weight8_1.close(); f_weight8_2.close();
// 		if (ZEROS)
// 		{
// 			generate_zeros("weight8_1", 1024*1152, temp);
// 			generate_zeros("weight8_2", 1024*1152, temp);
// 		}
// 		std::cout<<"code run here 972!"<<endl;
// 				/************************** FC - Weight9 **********************************/
// 		string path_weight9_1 = default_path+"weight9_"+to_string(partyNum);
// 		string path_weight9_2 = default_path+"weight9_"+to_string(nextParty(partyNum));
// 		ifstream f_weight9_1(path_weight9_1), f_weight9_2(path_weight9_2);

// 		for (int column = 0; column < 1024; ++column)
// 		{
// 			for (int row = 0; row < 1024; ++row)
// 			{
// 				f_weight9_1 >> temp_next; f_weight9_2 >> temp_prev;
// 				(*((FCLayer*)net->layers[17])->getWeights())[1024*row + column] = 
// 						std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
// 			}
// 		}
// 		f_weight9_1.close(); f_weight9_2.close();
// 		if (ZEROS)
// 		{
// 			generate_zeros("weight9_1", 1024 * 1024, temp);
// 			generate_zeros("weight9_2", 1024 * 1024, temp);
// 		}
// 		std::cout<<"code run here 988!"<<endl;
// 		/**************************FC - Weight10 **********************************/
// 		string path_weight10_1 = default_path+"weight10_"+to_string(partyNum);
// 		string path_weight10_2 = default_path+"weight10_"+to_string(nextParty(partyNum));
// 		ifstream f_weight10_1(path_weight10_1), f_weight10_2(path_weight10_2);

// 		for (int column = 0; column < 2; ++column)
// 		{
// 			for (int row = 0; row < 1024; ++row)
// 			{
// 				f_weight10_1 >> temp_next; f_weight10_2 >> temp_prev;
// 				(*((FCLayer*)net->layers[19])->getWeights())[2*row + column] = 
// 						std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
// 			}
// 		}
// 		f_weight10_1.close(); f_weight10_2.close();
// 		if (ZEROS)
// 		{
// 			generate_zeros("weight10_1", 2048, temp);
// 			generate_zeros("weight10_2", 2048, temp);
// 		}

// 		/**************************CNN - Bias1 **********************************/
// 		string path_bias1_1 = default_path+"bias1_"+to_string(partyNum);
// 		string path_bias1_2 = default_path+"bias1_"+to_string(nextParty(partyNum));
// 		ifstream f_bias1_1(path_bias1_1), f_bias1_2(path_bias1_2);

// 		for (int i = 0; i < 48; ++i)
// 		{
// 			f_bias1_1 >> temp_next; f_bias1_2 >> temp_prev;
// 			(*((CNNLayer*)net->layers[0])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
// 		}
// 		f_bias1_1.close(); f_bias1_2.close();
// 		if (ZEROS)
// 		{
// 			generate_zeros("bias1_1", 48, temp);
// 			generate_zeros("bias1_2", 48, temp);
// 		}

// 		/************************** BN - Bias2 **********************************/
// 		string path_bias2_1 = default_path+"bias2_"+to_string(partyNum);
// 		string path_bias2_2 = default_path+"bias2_"+to_string(nextParty(partyNum));
// 		ifstream f_bias2_1(path_bias2_1), f_bias2_2(path_bias2_2);

// 		for (int i = 0; i < 48; ++i)
// 		{
// 			f_bias2_1 >> temp_next; f_bias2_2 >> temp_prev;
// 			(*((BNLayer*)net->layers[3])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
// 		}
// 		f_bias2_1.close(); f_bias2_2.close();
// 		if (ZEROS)
// 		{
// 			generate_zeros("bias2_1", 48, temp);
// 			generate_zeros("bias2_2", 48, temp);
// 		} 

// 		/**************************CNN -  Bias3 **********************************/
// 		string path_bias3_1 = default_path+"bias3_"+to_string(partyNum);
// 		string path_bias3_2 = default_path+"bias3_"+to_string(nextParty(partyNum));
// 		ifstream f_bias3_1(path_bias3_1), f_bias3_2(path_bias3_2);

// 		for (int i = 0; i < 128; ++i)
// 		{
// 			f_bias3_1 >> temp_next; f_bias3_2 >> temp_prev;
// 			(*((CNNLayer*)net->layers[4])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
// 		}
// 		f_bias3_1.close(); f_bias3_2.close();
// 		if (ZEROS)
// 		{
// 			generate_zeros("bias3_1",128 , temp);
// 			generate_zeros("bias3_2", 128, temp);
// 		}
// 		std::cout<<"code run here 1068!"<<endl;
// 		/**************************BN -  Bias4 **********************************/
// 		string path_bias4_1 = default_path+"bias4_"+to_string(partyNum);
// 		string path_bias4_2 = default_path+"bias4_"+to_string(nextParty(partyNum));
// 		ifstream f_bias4_1(path_bias4_1), f_bias4_2(path_bias4_2);

// 		for (int i = 0; i < 128; ++i)
// 		{
// 			f_bias4_1 >> temp_next; f_bias4_2 >> temp_prev;
// 			(*((BNLayer*)net->layers[7])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
// 		}
// 		f_bias4_1.close(); f_bias4_2.close();
// 		if (ZEROS)
// 		{
// 			generate_zeros("bias4_1", 128, temp);
// 			generate_zeros("bias4_2", 128, temp);
// 		}

// 			/**************************	CNN -  Bias5 **********************************/
// 		string path_bias5_1 = default_path+"bias5_"+to_string(partyNum);
// 		string path_bias5_2 = default_path+"bias5_"+to_string(nextParty(partyNum));
// 		ifstream f_bias5_1(path_bias5_1), f_bias5_2(path_bias5_2);

// 		for (int i = 0; i < 192; ++i)
// 		{
// 			f_bias5_1 >> temp_next; f_bias5_2 >> temp_prev;
// 			(*((CNNLayer*)net->layers[8])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
// 		}
// 		f_bias5_1.close(); f_bias5_2.close();
// 		if (ZEROS)
// 		{
// 			generate_zeros("bias5_1", 192, temp);
// 			generate_zeros("bias5_2", 192, temp);
// 		}
// std::cout<<"code run here 1102!"<<endl;
// 		/************************** CNN - Bias6 **********************************/
// 		string path_bias6_1 = default_path+"bias6_"+to_string(partyNum);
// 		string path_bias6_2 = default_path+"bias6_"+to_string(nextParty(partyNum));
// 		ifstream f_bias6_1(path_bias6_1), f_bias6_2(path_bias6_2);

// 		for (int i = 0; i < 192; ++i)
// 		{
// 			f_bias6_1 >> temp_next; f_bias6_2 >> temp_prev;
// 			(*((CNNLayer*)net->layers[10])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
// 		}
// 		f_bias6_1.close(); f_bias6_2.close();
// 		if (ZEROS)
// 		{
// 			generate_zeros("bias6_1", 192, temp);
// 			generate_zeros("bias6_2", 192, temp);
// 		} 

// 		/************************** CNN - Bias7 **********************************/
// 		string path_bias7_1 = default_path+"bias7_"+to_string(partyNum);
// 		string path_bias7_2 = default_path+"bias7_"+to_string(nextParty(partyNum));
// 		ifstream f_bias7_1(path_bias7_1), f_bias7_2(path_bias7_2);

// 		for (int i = 0; i < 128; ++i)
// 		{
// 			f_bias7_1 >> temp_next; f_bias7_2 >> temp_prev;
// 			(*((CNNLayer*)net->layers[12])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
// 		}
// 		f_bias7_1.close(); f_bias7_2.close();
// 		if (ZEROS)
// 		{
// 			generate_zeros("bias7_1", 128, temp);
// 			generate_zeros("bias7_2", 128, temp);
// 		}

// 		/************************** CNN - Bias8 **********************************/
// 		string path_bias8_1 = default_path+"bias8_"+to_string(partyNum);
// 		string path_bias8_2 = default_path+"bias8_"+to_string(nextParty(partyNum));
// 		ifstream f_bias8_1(path_bias8_1), f_bias8_2(path_bias8_2);

// 		for (int i = 0; i < 1024; ++i)
// 		{
// 			f_bias8_1 >> temp_next; f_bias8_2 >> temp_prev;
// 			(*((FCLayer*)net->layers[15])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
// 		}
// 		f_bias8_1.close(); f_bias8_2.close();
// 		if (ZEROS)
// 		{
// 			generate_zeros("bias8_1", 1024, temp);
// 			generate_zeros("bias8_2", 1024, temp);
// 		}

// 			/**************************FC - Bias9 **********************************/
// 		string path_bias9_1 = default_path+"bias9_"+to_string(partyNum);
// 		string path_bias9_2 = default_path+"bias9_"+to_string(nextParty(partyNum));
// 		ifstream f_bias9_1(path_bias9_1), f_bias9_2(path_bias9_2);

// 		for (int i = 0; i < 1024; ++i)
// 		{
// 			f_bias9_1 >> temp_next; f_bias9_2 >> temp_prev;
// 			(*((FCLayer*)net->layers[17])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
// 		}
// 		f_bias9_1.close(); f_bias9_2.close();
// 		if (ZEROS)
// 		{
// 			generate_zeros("bias9_1", 1024, temp);
// 			generate_zeros("bias9_2", 1024, temp);
// 		}

// 		/**************************FC -  Bias10 **********************************/
// 		string path_bias10_1 = default_path+"bias10_"+to_string(partyNum);
// 		string path_bias10_2 = default_path+"bias10_"+to_string(nextParty(partyNum));
// 		ifstream f_bias10_1(path_bias10_1), f_bias10_2(path_bias10_2);

// 		for (int i = 0; i < 2; ++i)
// 		{
// 			f_bias10_1 >> temp_next; f_bias10_2 >> temp_prev;
// 			(*((FCLayer*)net->layers[19])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
// 		}
// 		f_bias10_1.close(); f_bias10_2.close();
// 		if (ZEROS)
// 		{
// 			generate_zeros("bias10_1", 2, temp);
// 			generate_zeros("bias10_2", 2, temp);
// 		} 


	}
	else if	(which_network(network).compare("RepVgg") == 0)//--ljf RepVgg
	{
			string temp = "RepVgg";
			/************************** Input **********************************/
			string path_input_1 = default_path+"input_"+to_string(partyNum);
			string path_input_2 = default_path+"input_"+to_string(nextParty(partyNum));
			ifstream f_input_1(path_input_1), f_input_2(path_input_2);

			for (int i = 0; i < INPUT_SIZE * MINI_BATCH_SIZE; ++i)
			{
				f_input_1 >> temp_next; f_input_2 >> temp_prev;
				net->inputData[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
			}
			f_input_1.close(); f_input_2.close();
			if (ZEROS)
			{
				generate_zeros("input_1", 225*225*3*64, temp);
				generate_zeros("input_2", 225*225*3*64, temp);
			}

		// 	/************************** labels for test --ljf**********************************/
		// string path_labels_1 = default_path+"train_labels_"+to_string(partyNum);
		// string path_labels_2 = default_path+"train_labels_"+to_string(nextParty(partyNum));
		// ifstream f_labels_1(path_labels_1), f_labels_2(path_labels_2);

		// for (int i = 0; i < LAST_LAYER_SIZE * MINI_BATCH_SIZE; ++i)
		// {
		// 	f_labels_1 >> temp_next; f_labels_2 >> temp_prev;
		// 	net->outputData[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		// }
		// f_labels_1.close(); f_labels_2.close();
		// if (ZEROS)
		// {
		// 	generate_zeros("train_labels_1", 128, temp);
		// 	generate_zeros("train_labels_2", 128, temp);//不需要onehot-coding
		// }
		// print_vector(net->outputData, "FLOAT", "outputData1:", 1280);
			/**************************CNN -  Weight1 **********************************/ 
		string path_weight1_1 = default_path+"weight1_"+to_string(partyNum);
		string path_weight1_2 = default_path+"weight1_"+to_string(nextParty(partyNum));
		ifstream f_weight1_1(path_weight1_1), f_weight1_2(path_weight1_2);

		for (int row = 0; row <3 * 3 * 3 * 48 ; ++row)
		{
			f_weight1_1 >> temp_next; f_weight1_2 >> temp_prev;
			(*((CNNLayer*)net->layers[0])->getWeights())[row] = 
					std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_weight1_1.close(); f_weight1_2.close();
		if (ZEROS)
		{
			generate_zeros("weight1_1",3 * 3 * 3 * 48, temp);
			generate_zeros("weight1_2",3 * 3 * 3 * 48, temp);
	
		}
		
				/**************************CNN -   Weight2 **********************************/
		string path_weight2_1 = default_path+"weight2_"+to_string(partyNum);
		string path_weight2_2 = default_path+"weight2_"+to_string(nextParty(partyNum));
		ifstream f_weight2_1(path_weight2_1), f_weight2_2(path_weight2_2);
			for (int row = 0; row < 3*3*48*48; ++row)
			{
				f_weight2_1 >> temp_next; f_weight2_2 >> temp_prev;
				(*((CNNLayer*)net->layers[2])->getWeights())[row] = //to do
						std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
			}
		f_weight2_1.close(); f_weight2_2.close();
		if (ZEROS)
		{
			generate_zeros("weight2_1", 3*3*48*48, temp);
			generate_zeros("weight2_2", 3*3*48*48, temp);
		}

			/**************************CNN -   Weight3 **********************************/
		
		string path_weight3_1 = default_path+"weight3_"+to_string(partyNum);
		string path_weight3_2 = default_path+"weight3_"+to_string(nextParty(partyNum));
		ifstream f_weight3_1(path_weight3_1), f_weight3_2(path_weight3_2);

		for (int row = 0; row < 3*3*48*48; ++row)
		{
			f_weight3_1 >> temp_next; f_weight3_2 >> temp_prev;
			(*((CNNLayer*)net->layers[4])->getWeights())[row] = 
					std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_weight3_1.close(); f_weight3_2.close();
		if (ZEROS)
		{
			generate_zeros("weight3_1",3*3*48*48, temp);
			generate_zeros("weight3_2", 3*3*48*48, temp);
		}

			/**************************  CNN - Weight4 **********************************/
		string path_weight4_1 = default_path+"weight4_"+to_string(partyNum);
		string path_weight4_2 = default_path+"weight4_"+to_string(nextParty(partyNum));
		ifstream f_weight4_1(path_weight4_1), f_weight4_2(path_weight4_2);

			for (int row = 0; row < 3*3*48*96; ++row)
			{
				f_weight4_1 >> temp_next; f_weight4_2 >> temp_prev;
				(*((CNNLayer*)net->layers[6])->getWeights())[row] = 
						std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
			}
		f_weight4_1.close(); f_weight4_2.close();
		if (ZEROS)
		{
			generate_zeros("weight4_1",  3*3*48*96, temp);
			generate_zeros("weight4_2",  3*3*48*96, temp);
		}

		/************************** CNN -  Weight5 **********************************/		
		string path_weight5_1 = default_path+"weight5_"+to_string(partyNum);
		string path_weight5_2 = default_path+"weight5_"+to_string(nextParty(partyNum));
		ifstream f_weight5_1(path_weight5_1), f_weight5_2(path_weight5_2);

		for (int row = 0; row < 3 * 3 * 96 * 96; ++row)
		{
			f_weight5_1 >> temp_next; f_weight5_2 >> temp_prev;
			(*((CNNLayer*)net->layers[8])->getWeights())[row] = 
					std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_weight5_1.close(); f_weight5_2.close();
		if (ZEROS)
		{
			generate_zeros("weight5_1",3 * 3 * 96 * 96, temp);
			generate_zeros("weight5_2",3 * 3 * 96 * 96, temp);
		}

		/**************************CNN -   Weight6 **********************************/
		string path_weight6_1 = default_path+"weight6_"+to_string(partyNum);
		string path_weight6_2 = default_path+"weight6_"+to_string(nextParty(partyNum));
		ifstream f_weight6_1(path_weight6_1), f_weight6_2(path_weight6_2);

		for (int row = 0; row < 3 * 3 * 96 * 96; ++row)
		{
			f_weight6_1 >> temp_next; f_weight6_2 >> temp_prev;
			(*((CNNLayer*)net->layers[10])->getWeights())[row] = 
					std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_weight6_1.close(); f_weight6_2.close();
		if (ZEROS)
		{
			generate_zeros("weight6_1",3 * 3 * 96 * 96, temp);
			generate_zeros("weight6_2", 3 * 3 * 96 * 96, temp);
		}

		/************************** CNN -  Weight7 **********************************/
		string path_weight7_1 = default_path+"weight7_"+to_string(partyNum);
		string path_weight7_2 = default_path+"weight7_"+to_string(nextParty(partyNum));
		ifstream f_weight7_1(path_weight7_1), f_weight7_2(path_weight7_2);

		for (int row = 0; row < 3 * 3 * 96 * 96; ++row)
		{
			f_weight7_1 >> temp_next; f_weight7_2 >> temp_prev;
			(*((CNNLayer*)net->layers[12])->getWeights())[row] = 
					std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_weight7_1.close(); f_weight7_2.close();
		if (ZEROS)
		{
			generate_zeros("weight7_1",3 * 3 * 96 * 96, temp);
			generate_zeros("weight7_2", 3 * 3 * 96 * 96, temp);
		}

		/************************** CNN -Weight8 **********************************/
		string path_weight8_1 = default_path+"weight8_"+to_string(partyNum);
		string path_weight8_2 = default_path+"weight8_"+to_string(nextParty(partyNum));
		ifstream f_weight8_1(path_weight8_1), f_weight8_2(path_weight8_2);

		for (int row = 0; row < 3 * 3 * 96 * 192; ++row)
		{
			f_weight8_1 >> temp_next;  f_weight8_2 >> temp_prev;
			(*((CNNLayer*)net->layers[14])->getWeights())[row] = 
					std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}

		f_weight8_1.close(); f_weight8_2.close();
		if (ZEROS)
		{
			generate_zeros("weight8_1", 3 * 3 * 96 * 192, temp);
			generate_zeros("weight8_2", 3 * 3 * 96 * 192, temp);
		}

				/************************** CNN - Weight9 **********************************/
		string path_weight9_1 = default_path+"weight9_"+to_string(partyNum);
		string path_weight9_2 = default_path+"weight9_"+to_string(nextParty(partyNum));
		ifstream f_weight9_1(path_weight9_1), f_weight9_2(path_weight9_2);

		for (int row = 0; row < 3 * 3 * 192 * 192; ++row)
		{
			f_weight9_1 >> temp_next;  f_weight9_2 >> temp_prev;
			(*((CNNLayer*)net->layers[16])->getWeights())[row] = 
					std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_weight9_1.close(); f_weight9_2.close();
		if (ZEROS)
		{
			generate_zeros("weight9_1", 3 * 3 * 192 * 192, temp);
			generate_zeros("weight9_2", 3 * 3 * 192 * 192, temp);
		}

		/**************************CNN - Weight10 **********************************/
		string path_weight10_1 = default_path+"weight10_"+to_string(partyNum);
		string path_weight10_2 = default_path+"weight10_"+to_string(nextParty(partyNum));
		ifstream f_weight10_1(path_weight10_1), f_weight10_2(path_weight10_2);

		for (int row = 0; row < 3 * 3 * 192 * 192; ++row)
		{
			f_weight10_1 >> temp_next;  f_weight10_2 >> temp_prev;
			(*((CNNLayer*)net->layers[18])->getWeights())[row] = 
					std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_weight10_1.close(); f_weight10_2.close();
		if (ZEROS)
		{
			generate_zeros("weight10_1",  3 * 3 * 192 * 192, temp);
			generate_zeros("weight10_2",  3 * 3 * 192 * 192, temp);
		}

		/**************************CNN -  Weight11 **********************************/ 
		string path_weight11_1 = default_path+"weight11_"+to_string(partyNum);
		string path_weight11_2 = default_path+"weight11_"+to_string(nextParty(partyNum));
		ifstream f_weight11_1(path_weight11_1), f_weight11_2(path_weight11_2);

		for (int row = 0; row <3 * 3 * 192 * 192 ; ++row)
		{
			f_weight11_1 >> temp_next; f_weight11_2 >> temp_prev;
			(*((CNNLayer*)net->layers[20])->getWeights())[row] = 
					std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_weight11_1.close(); f_weight11_2.close();
		if (ZEROS)
		{
			generate_zeros("weight11_1",3 * 3 * 192 * 192, temp);
			generate_zeros("weight11_2",3 * 3 * 192 * 192, temp);
		}

				/**************************CNN -   Weight12 **********************************/
		string path_weight12_1 = default_path+"weight12_"+to_string(partyNum);
		string path_weight12_2 = default_path+"weight12_"+to_string(nextParty(partyNum));
		ifstream f_weight12_1(path_weight12_1), f_weight12_2(path_weight12_2);
			for (int row = 0; row < 3 * 3 * 192 * 192; ++row)
			{
				f_weight12_1 >> temp_next; f_weight12_2 >> temp_prev;
				(*((CNNLayer*)net->layers[22])->getWeights())[row] = //to do
						std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
			}
		f_weight12_1.close(); f_weight12_2.close();
		if (ZEROS)
		{
			generate_zeros("weight12_1", 3 * 3 * 192 * 192, temp);
			generate_zeros("weight12_2", 3 * 3 * 192 * 192, temp);
		}

			/**************************CNN -   Weight13 **********************************/
		
		string path_weight13_1 = default_path+"weight13_"+to_string(partyNum);
		string path_weight13_2 = default_path+"weight13_"+to_string(nextParty(partyNum));
		ifstream f_weight13_1(path_weight13_1), f_weight13_2(path_weight13_2);

		for (int row = 0; row < 3 * 3 * 192 * 192; ++row)
		{
			f_weight13_1 >> temp_next; f_weight13_2 >> temp_prev;
			(*((CNNLayer*)net->layers[24])->getWeights())[row] = 
					std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_weight13_1.close(); f_weight13_2.close();
		if (ZEROS)
		{
			generate_zeros("weight13_1",3 * 3 * 192 * 192, temp);
			generate_zeros("weight13_2", 3 * 3 * 192 * 192, temp);
		}

			/**************************  CNN - Weight14 **********************************/
		string path_weight14_1 = default_path+"weight14_"+to_string(partyNum);
		string path_weight14_2 = default_path+"weight14_"+to_string(nextParty(partyNum));
		ifstream f_weight14_1(path_weight14_1), f_weight14_2(path_weight14_2);

			for (int row = 0; row < 3 * 3 * 192 * 192; ++row)
			{
				f_weight14_1 >> temp_next; f_weight14_2 >> temp_prev;
				(*((CNNLayer*)net->layers[26])->getWeights())[row] = 
						std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
			}
		f_weight14_1.close(); f_weight14_2.close();
		if (ZEROS)
		{
			generate_zeros("weight14_1",  3 * 3 * 192 * 192, temp);
			generate_zeros("weight14_2",  3 * 3 * 192 * 192, temp);
		}

		/************************** CNN -  Weight15 **********************************/		
		string path_weight15_1 = default_path+"weight15_"+to_string(partyNum);
		string path_weight15_2 = default_path+"weight15_"+to_string(nextParty(partyNum));
		ifstream f_weight15_1(path_weight15_1), f_weight15_2(path_weight15_2);

		for (int row = 0; row < 3 * 3 * 192 * 192; ++row)
		{
			f_weight15_1 >> temp_next; f_weight15_2 >> temp_prev;
			(*((CNNLayer*)net->layers[28])->getWeights())[row] = 
					std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_weight15_1.close(); f_weight15_2.close();
		if (ZEROS)
		{
			generate_zeros("weight15_1",3 * 3 * 192 * 192, temp);
			generate_zeros("weight15_2",3 * 3 * 192 * 192, temp);
		}

		/**************************CNN -   Weight16 **********************************/
		string path_weight16_1 = default_path+"weight16_"+to_string(partyNum);
		string path_weight16_2 = default_path+"weight16_"+to_string(nextParty(partyNum));
		ifstream f_weight16_1(path_weight16_1), f_weight16_2(path_weight16_2);

		for (int row = 0; row < 3 * 3 * 192 * 192; ++row)
		{
			f_weight16_1 >> temp_next; f_weight16_2 >> temp_prev;
			(*((CNNLayer*)net->layers[30])->getWeights())[row] = 
					std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_weight16_1.close(); f_weight16_2.close();
		if (ZEROS)
		{
			generate_zeros("weight16_1",3 * 3 * 192 * 192, temp);
			generate_zeros("weight16_2", 3 * 3 * 192 * 192, temp);
		}

		/************************** CNN -  Weight17 **********************************/
		string path_weight17_1 = default_path+"weight17_"+to_string(partyNum);
		string path_weight17_2 = default_path+"weight17_"+to_string(nextParty(partyNum));
		ifstream f_weight17_1(path_weight17_1), f_weight17_2(path_weight17_2);

		for (int row = 0; row < 3 * 3 * 192 * 192; ++row)
		{
			f_weight17_1 >> temp_next; f_weight17_2 >> temp_prev;
			(*((CNNLayer*)net->layers[32])->getWeights())[row] = 
					std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_weight17_1.close(); f_weight17_2.close();
		if (ZEROS)
		{
			generate_zeros("weight17_1",3 * 3 * 192 * 192, temp);
			generate_zeros("weight17_2", 3 * 3 * 192 * 192, temp);
		}

		/************************** CNN -Weight18 **********************************/
		string path_weight18_1 = default_path+"weight18_"+to_string(partyNum);
		string path_weight18_2 = default_path+"weight18_"+to_string(nextParty(partyNum));
		ifstream f_weight18_1(path_weight18_1), f_weight18_2(path_weight18_2);

		for (int row = 0; row <3 * 3 * 192 * 192; ++row)
		{
			f_weight18_1 >> temp_next;  f_weight18_2 >> temp_prev;
			(*((CNNLayer*)net->layers[34])->getWeights())[row] = 
					std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}

		f_weight18_1.close(); f_weight18_2.close();
		if (ZEROS)
		{
			generate_zeros("weight18_1", 3 * 3 * 192 * 192, temp);
			generate_zeros("weight18_2", 3 * 3 * 192 * 192, temp);
		}

				/************************** CNN - Weight19 **********************************/
		string path_weight19_1 = default_path+"weight19_"+to_string(partyNum);
		string path_weight19_2 = default_path+"weight19_"+to_string(nextParty(partyNum));
		ifstream f_weight19_1(path_weight19_1), f_weight19_2(path_weight19_2);

		for (int row = 0; row < 3 * 3 * 192 * 192; ++row)
		{
			f_weight19_1 >> temp_next;  f_weight19_2 >> temp_prev;
			(*((CNNLayer*)net->layers[36])->getWeights())[row] = 
					std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_weight19_1.close(); f_weight19_2.close();
		if (ZEROS)
		{
			generate_zeros("weight19_1", 3 * 3 * 192 * 192, temp);
			generate_zeros("weight19_2", 3 * 3 * 192 * 192, temp);
		}

		/**************************CNN - Weight20 **********************************/
		string path_weight20_1 = default_path+"weight20_"+to_string(partyNum);
		string path_weight20_2 = default_path+"weight20_"+to_string(nextParty(partyNum));
		ifstream f_weight20_1(path_weight20_1), f_weight20_2(path_weight20_2);

		for (int row = 0; row < 3 * 3 * 192 * 192; ++row)
		{
			f_weight20_1 >> temp_next;  f_weight20_2 >> temp_prev;
			(*((CNNLayer*)net->layers[38])->getWeights())[row] = 
					std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_weight20_1.close(); f_weight20_2.close();
		if (ZEROS)
		{
			generate_zeros("weight20_1",  3 * 3 * 192 * 192, temp);
			generate_zeros("weight20_2",  3 * 3 * 192 * 192, temp);
		}

				/**************************CNN - Weight21 **********************************/
		string path_weight21_1 = default_path+"weight21_"+to_string(partyNum);
		string path_weight21_2 = default_path+"weight21_"+to_string(nextParty(partyNum));
		ifstream f_weight21_1(path_weight21_1), f_weight21_2(path_weight21_2);

		for (int row = 0; row < 3 * 3 * 192 * 192; ++row)
		{
			f_weight21_1 >> temp_next;  f_weight21_2 >> temp_prev;
			(*((CNNLayer*)net->layers[40])->getWeights())[row] = 
					std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_weight21_1.close(); f_weight21_2.close();
		if (ZEROS)
		{
			generate_zeros("weight21_1",  3 * 3 * 192 * 192, temp);
			generate_zeros("weight21_2",  3 * 3 * 192 * 192, temp);
		}

				/**************************CNN - Weight22 **********************************/
		string path_weight22_1 = default_path+"weight22_"+to_string(partyNum);
		string path_weight22_2 = default_path+"weight22_"+to_string(nextParty(partyNum));
		ifstream f_weight22_1(path_weight22_1), f_weight22_2(path_weight22_2);

		for (int row = 0; row < 3 * 3 * 192 * 1024; ++row)
		{
			f_weight22_1 >> temp_next;  f_weight22_2 >> temp_prev;
			(*((CNNLayer*)net->layers[42])->getWeights())[row] = 
					std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_weight22_1.close(); f_weight22_2.close();
		if (ZEROS)
		{
			generate_zeros("weight22_1",  3 * 3 * 192 * 1024, temp);
			generate_zeros("weight22_2",  3 * 3 * 192 * 1024, temp);
		}

		/**************************FC -  Weight23**********************************/
		string path_weight23_1 = default_path+"weight23_"+to_string(partyNum);
		string path_weight23_2 = default_path+"weight23_"+to_string(nextParty(partyNum));	
		ifstream f_weight23_1(path_weight23_1), f_weight23_2(path_weight23_2);

		for (int column = 0; column < 2; ++column)
		{
			for (int row = 0; row < 1024; ++row)
			{
				f_weight23_1 >> temp_next; f_weight23_2 >> temp_prev;
				(*((FCLayer*)net->layers[45])->getWeights())[2*row + column] = 
						std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
			}
		}
		f_weight23_1.close(); f_weight23_2.close();
		if (ZEROS)
		{
			generate_zeros("weight23_1", 2*1024, temp);
			generate_zeros("weight23_2", 2*1024, temp);
		}

			/**************************CNN - Bias1 **********************************/
		string path_bias1_1 = default_path+"bias1_"+to_string(partyNum);
		string path_bias1_2 = default_path+"bias1_"+to_string(nextParty(partyNum));
		ifstream f_bias1_1(path_bias1_1), f_bias1_2(path_bias1_2);

		for (int i = 0; i < 48; ++i)
		{
			f_bias1_1 >> temp_next; f_bias1_2 >> temp_prev;
			(*((CNNLayer*)net->layers[0])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias1_1.close(); f_bias1_2.close();
		if (ZEROS)
		{
			generate_zeros("bias1_1", 48, temp);
			generate_zeros("bias1_2", 48, temp);
		}

		/************************** CNN  - Bias2 **********************************/
		string path_bias2_1 = default_path+"bias2_"+to_string(partyNum);
		string path_bias2_2 = default_path+"bias2_"+to_string(nextParty(partyNum));
		ifstream f_bias2_1(path_bias2_1), f_bias2_2(path_bias2_2);

		for (int i = 0; i < 48; ++i)
		{
			f_bias2_1 >> temp_next; f_bias2_2 >> temp_prev;
			(*((CNNLayer*)net->layers[2])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias2_1.close(); f_bias2_2.close();
		if (ZEROS)
		{
			generate_zeros("bias2_1", 48, temp);
			generate_zeros("bias2_2", 48, temp);
		} 

		/**************************CNN -  Bias3 **********************************/
		string path_bias3_1 = default_path+"bias3_"+to_string(partyNum);
		string path_bias3_2 = default_path+"bias3_"+to_string(nextParty(partyNum));
		ifstream f_bias3_1(path_bias3_1), f_bias3_2(path_bias3_2);

		for (int i = 0; i < 48; ++i)
		{
			f_bias3_1 >> temp_next; f_bias3_2 >> temp_prev;
			(*((CNNLayer*)net->layers[4])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias3_1.close(); f_bias3_2.close();
		if (ZEROS)
		{
			generate_zeros("bias3_1",48 , temp);
			generate_zeros("bias3_2", 48, temp);
		}

		/**************************CNN  -  Bias4 **********************************/
		string path_bias4_1 = default_path+"bias4_"+to_string(partyNum);
		string path_bias4_2 = default_path+"bias4_"+to_string(nextParty(partyNum));
		ifstream f_bias4_1(path_bias4_1), f_bias4_2(path_bias4_2);

		for (int i = 0; i < 96; ++i)
		{
			f_bias4_1 >> temp_next; f_bias4_2 >> temp_prev;
			(*((CNNLayer*)net->layers[6])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias4_1.close(); f_bias4_2.close();
		if (ZEROS)
		{
			generate_zeros("bias4_1", 96, temp);
			generate_zeros("bias4_2", 96, temp);
		}

			/**************************	CNN -  Bias5 **********************************/
		string path_bias5_1 = default_path+"bias5_"+to_string(partyNum);
		string path_bias5_2 = default_path+"bias5_"+to_string(nextParty(partyNum));
		ifstream f_bias5_1(path_bias5_1), f_bias5_2(path_bias5_2);

		for (int i = 0; i < 96; ++i)
		{
			f_bias5_1 >> temp_next; f_bias5_2 >> temp_prev;
			(*((CNNLayer*)net->layers[8])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias5_1.close(); f_bias5_2.close();
		if (ZEROS)
		{
			generate_zeros("bias5_1", 96, temp);
			generate_zeros("bias5_2", 96, temp);
		}

		/************************** CNN - Bias6 **********************************/
		string path_bias6_1 = default_path+"bias6_"+to_string(partyNum);
		string path_bias6_2 = default_path+"bias6_"+to_string(nextParty(partyNum));
		ifstream f_bias6_1(path_bias6_1), f_bias6_2(path_bias6_2);

		for (int i = 0; i < 96; ++i)
		{
			f_bias6_1 >> temp_next; f_bias6_2 >> temp_prev;
			(*((CNNLayer*)net->layers[10])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias6_1.close(); f_bias6_2.close();
		if (ZEROS)
		{
			generate_zeros("bias6_1", 96, temp);
			generate_zeros("bias6_2", 96, temp);
		} 

		/************************** CNN - Bias7 **********************************/
		string path_bias7_1 = default_path+"bias7_"+to_string(partyNum);
		string path_bias7_2 = default_path+"bias7_"+to_string(nextParty(partyNum));
		ifstream f_bias7_1(path_bias7_1), f_bias7_2(path_bias7_2);

		for (int i = 0; i < 96; ++i)
		{
			f_bias7_1 >> temp_next; f_bias7_2 >> temp_prev;
			(*((CNNLayer*)net->layers[12])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias7_1.close(); f_bias7_2.close();
		if (ZEROS)
		{
			generate_zeros("bias7_1", 96, temp);
			generate_zeros("bias7_2", 96, temp);
		}

		/************************** CNN - Bias8 **********************************/
		string path_bias8_1 = default_path+"bias8_"+to_string(partyNum);
		string path_bias8_2 = default_path+"bias8_"+to_string(nextParty(partyNum));
		ifstream f_bias8_1(path_bias8_1), f_bias8_2(path_bias8_2);

		for (int i = 0; i < 192; ++i)
		{
			f_bias8_1 >> temp_next; f_bias8_2 >> temp_prev;
			(*((CNNLayer*)net->layers[14])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias8_1.close(); f_bias8_2.close();
		if (ZEROS)
		{
			generate_zeros("bias8_1", 192, temp);
			generate_zeros("bias8_2", 192, temp);
		}

			/**************************CNN - Bias9 **********************************/
		string path_bias9_1 = default_path+"bias9_"+to_string(partyNum);
		string path_bias9_2 = default_path+"bias9_"+to_string(nextParty(partyNum));
		ifstream f_bias9_1(path_bias9_1), f_bias9_2(path_bias9_2);

		for (int i = 0; i < 192; ++i)
		{
			f_bias9_1 >> temp_next; f_bias9_2 >> temp_prev;
			(*((CNNLayer*)net->layers[16])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias9_1.close(); f_bias9_2.close();
		if (ZEROS)
		{
			generate_zeros("bias9_1", 192, temp);
			generate_zeros("bias9_2", 192, temp);
		}

		/**************************CNN -  Bias10 **********************************/
		string path_bias10_1 = default_path+"bias10_"+to_string(partyNum);
		string path_bias10_2 = default_path+"bias10_"+to_string(nextParty(partyNum));
		ifstream f_bias10_1(path_bias10_1), f_bias10_2(path_bias10_2);

		for (int i = 0; i < 192; ++i)
		{
			f_bias10_1 >> temp_next; f_bias10_2 >> temp_prev;
			(*((CNNLayer*)net->layers[18])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias10_1.close(); f_bias10_2.close();
		if (ZEROS)
		{
			generate_zeros("bias10_1", 192, temp);
			generate_zeros("bias10_2", 192, temp);
		} 

		/**************************CNN - Bias11**********************************/
		string path_bias11_1 = default_path+"bias11_"+to_string(partyNum);
		string path_bias11_2 = default_path+"bias11_"+to_string(nextParty(partyNum));
		ifstream f_bias11_1(path_bias11_1), f_bias11_2(path_bias11_2);

		for (int i = 0; i < 192; ++i)
		{
			f_bias11_1 >> temp_next; f_bias11_2 >> temp_prev;
			(*((CNNLayer*)net->layers[20])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias11_1.close(); f_bias11_2.close();
		if (ZEROS)
		{
			generate_zeros("bias11_1", 192, temp);
			generate_zeros("bias11_2", 192, temp);
		}

		/************************** CN - Bias12 **********************************/
		string path_bias12_1 = default_path+"bias12_"+to_string(partyNum);
		string path_bias12_2 = default_path+"bias12_"+to_string(nextParty(partyNum));
		ifstream f_bias12_1(path_bias12_1), f_bias12_2(path_bias12_2);

		for (int i = 0; i < 192; ++i)
		{
			f_bias12_1 >> temp_next; f_bias12_2 >> temp_prev;
			(*((CNNLayer*)net->layers[22])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias12_1.close(); f_bias12_2.close();
		if (ZEROS)
		{
			generate_zeros("bias12_1", 192, temp);
			generate_zeros("bias12_2", 192, temp);
		} 

		/**************************CNN -  Bias13 **********************************/
		string path_bias13_1 = default_path+"bias13_"+to_string(partyNum);
		string path_bias13_2 = default_path+"bias13_"+to_string(nextParty(partyNum));
		ifstream f_bias13_1(path_bias13_1), f_bias13_2(path_bias13_2);

		for (int i = 0; i < 192; ++i)
		{
			f_bias13_1 >> temp_next; f_bias13_2 >> temp_prev;
			(*((CNNLayer*)net->layers[24])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias13_1.close(); f_bias13_2.close();
		if (ZEROS)
		{
			generate_zeros("bias13_1",192 , temp);
			generate_zeros("bias13_2", 192, temp);
		}

		/**************************CNN -  Bias14 **********************************/
		string path_bias14_1 = default_path+"bias14_"+to_string(partyNum);
		string path_bias14_2 = default_path+"bias14_"+to_string(nextParty(partyNum));
		ifstream f_bias14_1(path_bias14_1), f_bias14_2(path_bias14_2);

		for (int i = 0; i < 192; ++i)
		{
			f_bias14_1 >> temp_next; f_bias14_2 >> temp_prev;
			(*((CNNLayer*)net->layers[26])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias14_1.close(); f_bias14_2.close();
		if (ZEROS)
		{
			generate_zeros("bias14_1", 192, temp);
			generate_zeros("bias14_2", 192, temp);
		}

			/**************************	CNN -  Bias15 **********************************/
		string path_bias15_1 = default_path+"bias15_"+to_string(partyNum);
		string path_bias15_2 = default_path+"bias15_"+to_string(nextParty(partyNum));
		ifstream f_bias15_1(path_bias15_1), f_bias15_2(path_bias15_2);

		for (int i = 0; i < 192; ++i)
		{
			f_bias15_1 >> temp_next; f_bias15_2 >> temp_prev;
			(*((CNNLayer*)net->layers[28])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias15_1.close(); f_bias15_2.close();
		if (ZEROS)
		{
			generate_zeros("bias15_1", 192, temp);
			generate_zeros("bias15_2", 192, temp);
		}

		/************************** CNN - Bias16 **********************************/
		string path_bias16_1 = default_path+"bias16_"+to_string(partyNum);
		string path_bias16_2 = default_path+"bias16_"+to_string(nextParty(partyNum));
		ifstream f_bias16_1(path_bias16_1), f_bias16_2(path_bias16_2);

		for (int i = 0; i < 192; ++i)
		{
			f_bias16_1 >> temp_next; f_bias16_2 >> temp_prev;
			(*((CNNLayer*)net->layers[30])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias16_1.close(); f_bias16_2.close();
		if (ZEROS)
		{
			generate_zeros("bias16_1", 192, temp);
			generate_zeros("bias16_2", 192, temp);
		} 

		/************************** CNN - Bias17 **********************************/
		string path_bias17_1 = default_path+"bias17_"+to_string(partyNum);
		string path_bias17_2 = default_path+"bias17_"+to_string(nextParty(partyNum));
		ifstream f_bias17_1(path_bias17_1), f_bias17_2(path_bias17_2);

		for (int i = 0; i < 192; ++i)
		{
			f_bias17_1 >> temp_next; f_bias17_2 >> temp_prev;
			(*((CNNLayer*)net->layers[32])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias17_1.close(); f_bias17_2.close();
		if (ZEROS)
		{
			generate_zeros("bias17_1", 192, temp);
			generate_zeros("bias17_2", 192, temp);
		}

		/************************** CNN - Bias18 **********************************/
		string path_bias18_1 = default_path+"bias18_"+to_string(partyNum);
		string path_bias18_2 = default_path+"bias18_"+to_string(nextParty(partyNum));
		ifstream f_bias18_1(path_bias18_1), f_bias18_2(path_bias18_2);

		for (int i = 0; i < 192; ++i)
		{
			f_bias18_1 >> temp_next; f_bias18_2 >> temp_prev;
			(*((CNNLayer*)net->layers[34])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias18_1.close(); f_bias18_2.close();
		if (ZEROS)
		{
			generate_zeros("bias18_1", 192, temp);
			generate_zeros("bias18_2", 192, temp);
		}

			/**************************CNN - Bias19 **********************************/
		string path_bias19_1 = default_path+"bias19_"+to_string(partyNum);
		string path_bias19_2 = default_path+"bias19_"+to_string(nextParty(partyNum));
		ifstream f_bias19_1(path_bias19_1), f_bias19_2(path_bias19_2);

		for (int i = 0; i < 192; ++i)
		{
			f_bias19_1 >> temp_next; f_bias19_2 >> temp_prev;
			(*((CNNLayer*)net->layers[36])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias19_1.close(); f_bias19_2.close();
		if (ZEROS)
		{
			generate_zeros("bias19_1", 192, temp);
			generate_zeros("bias19_2", 192, temp);
		}

		/**************************CNN -  Bias20 **********************************/
		string path_bias20_1 = default_path+"bias20_"+to_string(partyNum);
		string path_bias20_2 = default_path+"bias20_"+to_string(nextParty(partyNum));
		ifstream f_bias20_1(path_bias20_1), f_bias20_2(path_bias20_2);

		for (int i = 0; i < 192; ++i)
		{
			f_bias20_1 >> temp_next; f_bias20_2 >> temp_prev;
			(*((CNNLayer*)net->layers[38])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias20_1.close(); f_bias20_2.close();
		if (ZEROS)
		{
			generate_zeros("bias20_1", 192, temp);
			generate_zeros("bias20_2", 192, temp);
		} 

			/**************************CNN -  Bias21 **********************************/
		string path_bias21_1 = default_path+"bias21_"+to_string(partyNum);
		string path_bias21_2 = default_path+"bias21_"+to_string(nextParty(partyNum));
		ifstream f_bias21_1(path_bias21_1), f_bias21_2(path_bias21_2);

		for (int i = 0; i < 192; ++i)
		{
			f_bias21_1 >> temp_next; f_bias21_2 >> temp_prev;
			(*((CNNLayer*)net->layers[40])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias21_1.close(); f_bias21_2.close();
		if (ZEROS)
		{
			generate_zeros("bias21_1", 192, temp);
			generate_zeros("bias21_2", 192, temp);
		} 

			/**************************CNN -  Bias22 **********************************/
		string path_bias22_1 = default_path+"bias22_"+to_string(partyNum);
		string path_bias22_2 = default_path+"bias22_"+to_string(nextParty(partyNum));
		ifstream f_bias22_1(path_bias22_1), f_bias22_2(path_bias22_2);

		for (int i = 0; i < 1024; ++i)
		{
			f_bias22_1 >> temp_next; f_bias22_2 >> temp_prev;
			(*((CNNLayer*)net->layers[42])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias20_1.close(); f_bias20_2.close();
		if (ZEROS)
		{
			generate_zeros("bias22_1", 1024, temp);
			generate_zeros("bias22_2", 1024, temp);
		} 

		/**************************FC -  Bias23 **********************************/
		string path_bias23_1 = default_path+"bias23_"+to_string(partyNum);
		string path_bias23_2 = default_path+"bias23_"+to_string(nextParty(partyNum));
		ifstream f_bias23_1(path_bias23_1), f_bias23_2(path_bias23_2);

		for (int i = 0; i < 2; ++i)
		{
			f_bias23_1 >> temp_next; f_bias23_2 >> temp_prev;
			(*((FCLayer*)net->layers[45])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias23_1.close(); f_bias23_2.close();
		if (ZEROS)
		{
			generate_zeros("bias23_1", 2, temp);
			generate_zeros("bias23_2", 2, temp);
		} 

	}
	else if (which_network(network).compare("VGG16") == 0)//--ljf VGG16
	{
		string temp = "VGG16";
			/************************** Input **********************************/
			string path_input_1 = default_path+"input_"+to_string(partyNum);
			string path_input_2 = default_path+"input_"+to_string(nextParty(partyNum));
			ifstream f_input_1(path_input_1), f_input_2(path_input_2);

			for (int i = 0; i < INPUT_SIZE * MINI_BATCH_SIZE; ++i)
			{
				f_input_1 >> temp_next; f_input_2 >> temp_prev;
				net->inputData[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
			}
			f_input_1.close(); f_input_2.close();
			if (ZEROS)
			{
				generate_zeros("input_1", 224*224*3*2, temp);
				generate_zeros("input_2", 224*224*3*2, temp);
			}

		// 	/************************** labels for test --ljf**********************************/
		// string path_labels_1 = default_path+"train_labels_"+to_string(partyNum);
		// string path_labels_2 = default_path+"train_labels_"+to_string(nextParty(partyNum));
		// ifstream f_labels_1(path_labels_1), f_labels_2(path_labels_2);

		// for (int i = 0; i < LAST_LAYER_SIZE * MINI_BATCH_SIZE; ++i)
		// {
		// 	f_labels_1 >> temp_next; f_labels_2 >> temp_prev;
		// 	net->outputData[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		// }
		// f_labels_1.close(); f_labels_2.close();
		// if (ZEROS)
		// {
		// 	generate_zeros("train_labels_1", 128, temp);
		// 	generate_zeros("train_labels_2", 128, temp);//不需要onehot-coding
		// }
		// print_vector(net->outputData, "FLOAT", "outputData1:", 1280);
			/**************************CNN -  Weight1 **********************************/ 
		string path_weight1_1 = default_path+"weight1_"+to_string(partyNum);
		string path_weight1_2 = default_path+"weight1_"+to_string(nextParty(partyNum));
		ifstream f_weight1_1(path_weight1_1), f_weight1_2(path_weight1_2);

		for (int row = 0; row <3 * 3 * 3 * 64 ; ++row)
		{
			f_weight1_1 >> temp_next; f_weight1_2 >> temp_prev;
			(*((CNNLayer*)net->layers[0])->getWeights())[row] = 
					std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_weight1_1.close(); f_weight1_2.close();
		if (ZEROS)
		{
			generate_zeros("weight1_1",3 * 3 * 3 * 64, temp);
			generate_zeros("weight1_2",3 * 3 * 3 * 64, temp);
	
		}
		
				/**************************CNN -   Weight2 **********************************/
		string path_weight2_1 = default_path+"weight2_"+to_string(partyNum);
		string path_weight2_2 = default_path+"weight2_"+to_string(nextParty(partyNum));
		ifstream f_weight2_1(path_weight2_1), f_weight2_2(path_weight2_2);
			for (int row = 0; row < 3*3*64*64; ++row)
			{
				f_weight2_1 >> temp_next; f_weight2_2 >> temp_prev;
				(*((CNNLayer*)net->layers[2])->getWeights())[row] = //to do
						std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
			}
		f_weight2_1.close(); f_weight2_2.close();
		if (ZEROS)
		{
			generate_zeros("weight2_1", 3*3*64*64, temp);
			generate_zeros("weight2_2", 3*3*64*64, temp);
		}

			/**************************CNN -   Weight3 **********************************/
		
		string path_weight3_1 = default_path+"weight3_"+to_string(partyNum);
		string path_weight3_2 = default_path+"weight3_"+to_string(nextParty(partyNum));
		ifstream f_weight3_1(path_weight3_1), f_weight3_2(path_weight3_2);

		for (int row = 0; row < 3*3*128*64; ++row)
		{
			f_weight3_1 >> temp_next; f_weight3_2 >> temp_prev;
			(*((CNNLayer*)net->layers[5])->getWeights())[row] = 
					std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_weight3_1.close(); f_weight3_2.close();
		if (ZEROS)
		{
			generate_zeros("weight3_1",3*3*128*64, temp);
			generate_zeros("weight3_2", 3*3*128*64, temp);
		}

			/**************************  CNN - Weight4 **********************************/
		string path_weight4_1 = default_path+"weight4_"+to_string(partyNum);
		string path_weight4_2 = default_path+"weight4_"+to_string(nextParty(partyNum));
		ifstream f_weight4_1(path_weight4_1), f_weight4_2(path_weight4_2);

			for (int row = 0; row < 3*3*128*128; ++row)
			{
				f_weight4_1 >> temp_next; f_weight4_2 >> temp_prev;
				(*((CNNLayer*)net->layers[7])->getWeights())[row] = 
						std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
			}
		f_weight4_1.close(); f_weight4_2.close();
		if (ZEROS)
		{
			generate_zeros("weight4_1",  3*3*128*128, temp);
			generate_zeros("weight4_2",  3*3*128*128, temp);
		}

		/************************** CNN -  Weight5 **********************************/		
		string path_weight5_1 = default_path+"weight5_"+to_string(partyNum);
		string path_weight5_2 = default_path+"weight5_"+to_string(nextParty(partyNum));
		ifstream f_weight5_1(path_weight5_1), f_weight5_2(path_weight5_2);

		for (int row = 0; row < 3*3*256*128; ++row)
		{
			f_weight5_1 >> temp_next; f_weight5_2 >> temp_prev;
			(*((CNNLayer*)net->layers[10])->getWeights())[row] = 
					std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_weight5_1.close(); f_weight5_2.close();
		if (ZEROS)
		{
			generate_zeros("weight5_1",3*3*256*128, temp);
			generate_zeros("weight5_2",3*3*256*128, temp);
		}

		/**************************CNN -   Weight6 **********************************/
		string path_weight6_1 = default_path+"weight6_"+to_string(partyNum);
		string path_weight6_2 = default_path+"weight6_"+to_string(nextParty(partyNum));
		ifstream f_weight6_1(path_weight6_1), f_weight6_2(path_weight6_2);

		for (int row = 0; row <3*3*256*256; ++row)
		{
			f_weight6_1 >> temp_next; f_weight6_2 >> temp_prev;
			(*((CNNLayer*)net->layers[12])->getWeights())[row] = 
					std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_weight6_1.close(); f_weight6_2.close();
		if (ZEROS)
		{
			generate_zeros("weight6_1",3*3*256*256, temp);
			generate_zeros("weight6_2", 3*3*256*256, temp);
		}

		/************************** CNN -  Weight7 **********************************/
		string path_weight7_1 = default_path+"weight7_"+to_string(partyNum);
		string path_weight7_2 = default_path+"weight7_"+to_string(nextParty(partyNum));
		ifstream f_weight7_1(path_weight7_1), f_weight7_2(path_weight7_2);

		for (int row = 0; row < 3*3*256*256; ++row)
		{
			f_weight7_1 >> temp_next; f_weight7_2 >> temp_prev;
			(*((CNNLayer*)net->layers[14])->getWeights())[row] = 
					std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_weight7_1.close(); f_weight7_2.close();
		if (ZEROS)
		{
			generate_zeros("weight7_1",3*3*256*256, temp);
			generate_zeros("weight7_2",3*3*256*256, temp);
		}

		/************************** CNN -Weight8 **********************************/
		string path_weight8_1 = default_path+"weight8_"+to_string(partyNum);
		string path_weight8_2 = default_path+"weight8_"+to_string(nextParty(partyNum));
		ifstream f_weight8_1(path_weight8_1), f_weight8_2(path_weight8_2);

		for (int row = 0; row < 3*3*512*256; ++row)
		{
			f_weight8_1 >> temp_next;  f_weight8_2 >> temp_prev;
			(*((CNNLayer*)net->layers[17])->getWeights())[row] = 
					std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}

		f_weight8_1.close(); f_weight8_2.close();
		if (ZEROS)
		{
			generate_zeros("weight8_1", 3*3*512*256, temp);
			generate_zeros("weight8_2", 3*3*512*256, temp);
		}

				/************************** CNN - Weight9 **********************************/
		string path_weight9_1 = default_path+"weight9_"+to_string(partyNum);
		string path_weight9_2 = default_path+"weight9_"+to_string(nextParty(partyNum));
		ifstream f_weight9_1(path_weight9_1), f_weight9_2(path_weight9_2);

		for (int row = 0; row <3*3*512*512; ++row)
		{
			f_weight9_1 >> temp_next;  f_weight9_2 >> temp_prev;
			(*((CNNLayer*)net->layers[19])->getWeights())[row] = 
					std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_weight9_1.close(); f_weight9_2.close();
		if (ZEROS)
		{
			generate_zeros("weight9_1", 3*3*512*512, temp);
			generate_zeros("weight9_2", 3*3*512*512, temp);
		}

		/**************************CNN - Weight10 **********************************/
		string path_weight10_1 = default_path+"weight10_"+to_string(partyNum);
		string path_weight10_2 = default_path+"weight10_"+to_string(nextParty(partyNum));
		ifstream f_weight10_1(path_weight10_1), f_weight10_2(path_weight10_2);

		for (int row = 0; row < 3*3*512*512; ++row)
		{
			f_weight10_1 >> temp_next;  f_weight10_2 >> temp_prev;
			(*((CNNLayer*)net->layers[21])->getWeights())[row] = 
					std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_weight10_1.close(); f_weight10_2.close();
		if (ZEROS)
		{
			generate_zeros("weight10_1",  3*3*512*512, temp);
			generate_zeros("weight10_2",  3*3*512*512, temp);
		}

		/**************************CNN -  Weight11 **********************************/ 
		string path_weight11_1 = default_path+"weight11_"+to_string(partyNum);
		string path_weight11_2 = default_path+"weight11_"+to_string(nextParty(partyNum));
		ifstream f_weight11_1(path_weight11_1), f_weight11_2(path_weight11_2);

		for (int row = 0; row <3*3*512*512 ; ++row)
		{
			f_weight11_1 >> temp_next; f_weight11_2 >> temp_prev;
			(*((CNNLayer*)net->layers[24])->getWeights())[row] = 
					std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_weight11_1.close(); f_weight11_2.close();
		if (ZEROS)
		{
			generate_zeros("weight11_1",3*3*512*512, temp);
			generate_zeros("weight11_2",3*3*512*512, temp);
		}

				/**************************CNN -   Weight12 **********************************/
		string path_weight12_1 = default_path+"weight12_"+to_string(partyNum);
		string path_weight12_2 = default_path+"weight12_"+to_string(nextParty(partyNum));
		ifstream f_weight12_1(path_weight12_1), f_weight12_2(path_weight12_2);
			for (int row = 0; row < 3*3*512*512; ++row)
			{
				f_weight12_1 >> temp_next; f_weight12_2 >> temp_prev;
				(*((CNNLayer*)net->layers[26])->getWeights())[row] = //to do
						std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
			}
		f_weight12_1.close(); f_weight12_2.close();
		if (ZEROS)
		{
			generate_zeros("weight12_1", 3*3*512*512, temp);
			generate_zeros("weight12_2", 3*3*512*512, temp);
		}

			/**************************CNN -   Weight13 **********************************/
		
		string path_weight13_1 = default_path+"weight13_"+to_string(partyNum);
		string path_weight13_2 = default_path+"weight13_"+to_string(nextParty(partyNum));
		ifstream f_weight13_1(path_weight13_1), f_weight13_2(path_weight13_2);

		for (int row = 0; row < 3*3*512*512; ++row)
		{
			f_weight13_1 >> temp_next; f_weight13_2 >> temp_prev;
			(*((CNNLayer*)net->layers[28])->getWeights())[row] = 
					std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_weight13_1.close(); f_weight13_2.close();
		if (ZEROS)
		{
			generate_zeros("weight13_1",3*3*512*512, temp);
			generate_zeros("weight13_2", 3*3*512*512, temp);
		}

			/**************************  FC - Weight14 **********************************/
		string path_weight14_1 = default_path+"weight14_"+to_string(partyNum);
		string path_weight14_2 = default_path+"weight14_"+to_string(nextParty(partyNum));
		ifstream f_weight14_1(path_weight14_1), f_weight14_2(path_weight14_2);


		for (int column = 0; column <4096; ++column)
		{
			for (int row = 0; row < 25088; ++row)
			{
				f_weight14_1 >> temp_next; f_weight14_2 >> temp_prev;
				(*((FCLayer*)net->layers[31])->getWeights())[4096*row + column] = 
						std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
			}
		}
		f_weight14_1.close(); f_weight14_2.close();
		if (ZEROS)
		{
			generate_zeros("weight14_1",   25088 * 4096, temp);
			generate_zeros("weight14_2",   25088 * 4096, temp);
		}

		/************************** FC -  Weight15 **********************************/		
		string path_weight15_1 = default_path+"weight15_"+to_string(partyNum);
		string path_weight15_2 = default_path+"weight15_"+to_string(nextParty(partyNum));
		ifstream f_weight15_1(path_weight15_1), f_weight15_2(path_weight15_2);


		for (int column = 0; column < 4096; ++column)
		{
			for (int row = 0; row < 4096; ++row)
			{
				f_weight15_1 >> temp_next; f_weight15_2 >> temp_prev;
				(*((FCLayer*)net->layers[33])->getWeights())[4096*row + column] = 
						std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
			}
		}
		f_weight15_1.close(); f_weight15_2.close();
		if (ZEROS)
		{
			generate_zeros("weight15_1",4096* 4096, temp);
			generate_zeros("weight15_2",4096* 4096, temp);
		}

		/**************************FC -   Weight16 **********************************/
		string path_weight16_1 = default_path+"weight16_"+to_string(partyNum);
		string path_weight16_2 = default_path+"weight16_"+to_string(nextParty(partyNum));
		ifstream f_weight16_1(path_weight16_1), f_weight16_2(path_weight16_2);


		for (int column = 0; column < 2; ++column)
		{
			for (int row = 0; row < 4096; ++row)
			{
				f_weight16_1 >> temp_next; f_weight16_2 >> temp_prev;
				(*((FCLayer*)net->layers[35])->getWeights())[2*row + column] = 
						std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
			}
		}
		f_weight16_1.close(); f_weight16_2.close();
		if (ZEROS)
		{
			generate_zeros("weight16_1",4096 * 2, temp);
			generate_zeros("weight16_2",4096 * 2, temp);
		}


			/**************************CNN - Bias1 **********************************/
		string path_bias1_1 = default_path+"bias1_"+to_string(partyNum);
		string path_bias1_2 = default_path+"bias1_"+to_string(nextParty(partyNum));
		ifstream f_bias1_1(path_bias1_1), f_bias1_2(path_bias1_2);

		for (int i = 0; i < 64; ++i)
		{
			f_bias1_1 >> temp_next; f_bias1_2 >> temp_prev;
			(*((CNNLayer*)net->layers[0])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias1_1.close(); f_bias1_2.close();
		if (ZEROS)
		{
			generate_zeros("bias1_1", 64, temp);
			generate_zeros("bias1_2", 64, temp);
		}

		/************************** CNN  - Bias2 **********************************/
		string path_bias2_1 = default_path+"bias2_"+to_string(partyNum);
		string path_bias2_2 = default_path+"bias2_"+to_string(nextParty(partyNum));
		ifstream f_bias2_1(path_bias2_1), f_bias2_2(path_bias2_2);

		for (int i = 0; i < 64; ++i)
		{
			f_bias2_1 >> temp_next; f_bias2_2 >> temp_prev;
			(*((CNNLayer*)net->layers[2])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias2_1.close(); f_bias2_2.close();
		if (ZEROS)
		{
			generate_zeros("bias2_1", 64, temp);
			generate_zeros("bias2_2", 64, temp);
		} 

		/**************************CNN -  Bias3 **********************************/
		string path_bias3_1 = default_path+"bias3_"+to_string(partyNum);
		string path_bias3_2 = default_path+"bias3_"+to_string(nextParty(partyNum));
		ifstream f_bias3_1(path_bias3_1), f_bias3_2(path_bias3_2);

		for (int i = 0; i < 128; ++i)
		{
			f_bias3_1 >> temp_next; f_bias3_2 >> temp_prev;
			(*((CNNLayer*)net->layers[5])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias3_1.close(); f_bias3_2.close();
		if (ZEROS)
		{
			generate_zeros("bias3_1",128 , temp);
			generate_zeros("bias3_2", 128, temp);
		}

		/**************************CNN  -  Bias4 **********************************/
		string path_bias4_1 = default_path+"bias4_"+to_string(partyNum);
		string path_bias4_2 = default_path+"bias4_"+to_string(nextParty(partyNum));
		ifstream f_bias4_1(path_bias4_1), f_bias4_2(path_bias4_2);

		for (int i = 0; i < 128; ++i)
		{
			f_bias4_1 >> temp_next; f_bias4_2 >> temp_prev;
			(*((CNNLayer*)net->layers[7])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias4_1.close(); f_bias4_2.close();
		if (ZEROS)
		{
			generate_zeros("bias4_1", 128, temp);
			generate_zeros("bias4_2", 128, temp);
		}

			/**************************	CNN -  Bias5 **********************************/
		string path_bias5_1 = default_path+"bias5_"+to_string(partyNum);
		string path_bias5_2 = default_path+"bias5_"+to_string(nextParty(partyNum));
		ifstream f_bias5_1(path_bias5_1), f_bias5_2(path_bias5_2);

		for (int i = 0; i < 256; ++i)
		{
			f_bias5_1 >> temp_next; f_bias5_2 >> temp_prev;
			(*((CNNLayer*)net->layers[10])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias5_1.close(); f_bias5_2.close();
		if (ZEROS)
		{
			generate_zeros("bias5_1", 256, temp);
			generate_zeros("bias5_2", 256, temp);
		}

		/************************** CNN - Bias6 **********************************/
		string path_bias6_1 = default_path+"bias6_"+to_string(partyNum);
		string path_bias6_2 = default_path+"bias6_"+to_string(nextParty(partyNum));
		ifstream f_bias6_1(path_bias6_1), f_bias6_2(path_bias6_2);

		for (int i = 0; i < 256; ++i)
		{
			f_bias6_1 >> temp_next; f_bias6_2 >> temp_prev;
			(*((CNNLayer*)net->layers[12])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias6_1.close(); f_bias6_2.close();
		if (ZEROS)
		{
			generate_zeros("bias6_1", 256, temp);
			generate_zeros("bias6_2", 256, temp);
		} 

		/************************** CNN - Bias7 **********************************/
		string path_bias7_1 = default_path+"bias7_"+to_string(partyNum);
		string path_bias7_2 = default_path+"bias7_"+to_string(nextParty(partyNum));
		ifstream f_bias7_1(path_bias7_1), f_bias7_2(path_bias7_2);

		for (int i = 0; i < 256; ++i)
		{
			f_bias7_1 >> temp_next; f_bias7_2 >> temp_prev;
			(*((CNNLayer*)net->layers[14])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias7_1.close(); f_bias7_2.close();
		if (ZEROS)
		{
			generate_zeros("bias7_1", 256, temp);
			generate_zeros("bias7_2", 256, temp);
		}

		/************************** CNN - Bias8 **********************************/
		string path_bias8_1 = default_path+"bias8_"+to_string(partyNum);
		string path_bias8_2 = default_path+"bias8_"+to_string(nextParty(partyNum));
		ifstream f_bias8_1(path_bias8_1), f_bias8_2(path_bias8_2);

		for (int i = 0; i < 512; ++i)
		{
			f_bias8_1 >> temp_next; f_bias8_2 >> temp_prev;
			(*((CNNLayer*)net->layers[17])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias8_1.close(); f_bias8_2.close();
		if (ZEROS)
		{
			generate_zeros("bias8_1", 512, temp);
			generate_zeros("bias8_2", 512, temp);
		}

			/**************************CNN - Bias9 **********************************/
		string path_bias9_1 = default_path+"bias9_"+to_string(partyNum);
		string path_bias9_2 = default_path+"bias9_"+to_string(nextParty(partyNum));
		ifstream f_bias9_1(path_bias9_1), f_bias9_2(path_bias9_2);

		for (int i = 0; i < 512; ++i)
		{
			f_bias9_1 >> temp_next; f_bias9_2 >> temp_prev;
			(*((CNNLayer*)net->layers[19])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias9_1.close(); f_bias9_2.close();
		if (ZEROS)
		{
			generate_zeros("bias9_1", 512, temp);
			generate_zeros("bias9_2", 512, temp);
		}

		/**************************CNN -  Bias10 **********************************/
		string path_bias10_1 = default_path+"bias10_"+to_string(partyNum);
		string path_bias10_2 = default_path+"bias10_"+to_string(nextParty(partyNum));
		ifstream f_bias10_1(path_bias10_1), f_bias10_2(path_bias10_2);

		for (int i = 0; i < 512; ++i)
		{
			f_bias10_1 >> temp_next; f_bias10_2 >> temp_prev;
			(*((CNNLayer*)net->layers[21])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias10_1.close(); f_bias10_2.close();
		if (ZEROS)
		{
			generate_zeros("bias10_1", 512, temp);
			generate_zeros("bias10_2", 512, temp);
		} 

		/**************************CNN - Bias11**********************************/
		string path_bias11_1 = default_path+"bias11_"+to_string(partyNum);
		string path_bias11_2 = default_path+"bias11_"+to_string(nextParty(partyNum));
		ifstream f_bias11_1(path_bias11_1), f_bias11_2(path_bias11_2);

		for (int i = 0; i < 512; ++i)
		{
			f_bias11_1 >> temp_next; f_bias11_2 >> temp_prev;
			(*((CNNLayer*)net->layers[24])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias11_1.close(); f_bias11_2.close();
		if (ZEROS)
		{
			generate_zeros("bias11_1", 512, temp);
			generate_zeros("bias11_2", 512, temp);
		}

		/************************** CN - Bias12 **********************************/
		string path_bias12_1 = default_path+"bias12_"+to_string(partyNum);
		string path_bias12_2 = default_path+"bias12_"+to_string(nextParty(partyNum));
		ifstream f_bias12_1(path_bias12_1), f_bias12_2(path_bias12_2);

		for (int i = 0; i < 512; ++i)
		{
			f_bias12_1 >> temp_next; f_bias12_2 >> temp_prev;
			(*((CNNLayer*)net->layers[26])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias12_1.close(); f_bias12_2.close();
		if (ZEROS)
		{
			generate_zeros("bias12_1", 512, temp);
			generate_zeros("bias12_2", 512, temp);
		} 

		/**************************CNN -  Bias13 **********************************/
		string path_bias13_1 = default_path+"bias13_"+to_string(partyNum);
		string path_bias13_2 = default_path+"bias13_"+to_string(nextParty(partyNum));
		ifstream f_bias13_1(path_bias13_1), f_bias13_2(path_bias13_2);

		for (int i = 0; i < 512; ++i)
		{
			f_bias13_1 >> temp_next; f_bias13_2 >> temp_prev;
			(*((CNNLayer*)net->layers[28])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias13_1.close(); f_bias13_2.close();
		if (ZEROS)
		{
			generate_zeros("bias13_1",512 , temp);
			generate_zeros("bias13_2", 512, temp);
		}

		/**************************FC -  Bias14 **********************************/
		string path_bias14_1 = default_path+"bias14_"+to_string(partyNum);
		string path_bias14_2 = default_path+"bias14_"+to_string(nextParty(partyNum));
		ifstream f_bias14_1(path_bias14_1), f_bias14_2(path_bias14_2);

		for (int i = 0; i < 4096; ++i)
		{
			f_bias14_1 >> temp_next; f_bias14_2 >> temp_prev;
			(*((FCLayer*)net->layers[31])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias14_1.close(); f_bias14_2.close();
		if (ZEROS)
		{
			generate_zeros("bias14_1", 4096, temp);
			generate_zeros("bias14_2", 4096, temp);
		}

			/**************************	FC -  Bias15 **********************************/
		string path_bias15_1 = default_path+"bias15_"+to_string(partyNum);
		string path_bias15_2 = default_path+"bias15_"+to_string(nextParty(partyNum));
		ifstream f_bias15_1(path_bias15_1), f_bias15_2(path_bias15_2);

		for (int i = 0; i < 4096; ++i)
		{
			f_bias15_1 >> temp_next; f_bias15_2 >> temp_prev;
			(*((FCLayer*)net->layers[33])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias15_1.close(); f_bias15_2.close();
		if (ZEROS)
		{
			generate_zeros("bias15_1", 4096, temp);
			generate_zeros("bias15_2", 4096, temp);
		}

		/************************** FC - Bias16 **********************************/
		string path_bias16_1 = default_path+"bias16_"+to_string(partyNum);
		string path_bias16_2 = default_path+"bias16_"+to_string(nextParty(partyNum));
		ifstream f_bias16_1(path_bias16_1), f_bias16_2(path_bias16_2);

		for (int i = 0; i < 2; ++i)
		{
			f_bias16_1 >> temp_next; f_bias16_2 >> temp_prev;
			(*((FCLayer*)net->layers[35])->getBias())[i] = std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev));
		}
		f_bias16_1.close(); f_bias16_2.close();
		if (ZEROS)
		{
			generate_zeros("bias16_1", 2, temp);
			generate_zeros("bias16_2", 2, temp);
		} 


	}
	else 
		error("Preloading network error");
	cout << "Preloading completed..." << endl;
}

void loadData(string net, string dataset)
{
	if (dataset.compare("MNIST") == 0)
	{
		INPUT_SIZE = 784;
		LAST_LAYER_SIZE = 10;
		TRAINING_DATA_SIZE = 8;
		TEST_DATA_SIZE = 8;
		LARGE_NETWORK = false;
	}
	else if (dataset.compare("DIAMOND") == 0)// --ljf
	{
		LARGE_NETWORK = true;
		if (net.compare("AlexNet") == 0)
		{
		INPUT_SIZE = 227*227*3;
		LAST_LAYER_SIZE = 2;
		TRAINING_DATA_SIZE = 8;
		TEST_DATA_SIZE = 8;
		}		
		else if (net.compare("RepVgg") == 0)
		{
		INPUT_SIZE = 225*225*3;
		LAST_LAYER_SIZE = 2;
		TRAINING_DATA_SIZE = 8;
		TEST_DATA_SIZE = 8;
		}
		else if (net.compare("VGG16") == 0)
		{
		INPUT_SIZE = 224*224*3;
		LAST_LAYER_SIZE = 2;
		TRAINING_DATA_SIZE = 8;
		TEST_DATA_SIZE = 8;
		}
		else
		assert(false && "Only AlexNet,RepVgg and VGG16 supported on DIAMOND");
	}
	else if (dataset.compare("CIFAR10") == 0)
	{
		LARGE_NETWORK = false;
		if (net.compare("AlexNet") == 0)
		{
			INPUT_SIZE = 33*33*3;
			LAST_LAYER_SIZE = 10;
			TRAINING_DATA_SIZE = 8;
			TEST_DATA_SIZE = 8;			
		}
		else if (net.compare("VGG16") == 0)
		{
			INPUT_SIZE = 32*32*3;
			LAST_LAYER_SIZE = 10;
			TRAINING_DATA_SIZE = 8;
			TEST_DATA_SIZE = 8;	
		}
		else
			assert(false && "Only AlexNet and VGG16 supported on CIFAR10");
	}
	else if (dataset.compare("ImageNet") == 0)
	{
		LARGE_NETWORK = true;
		//https://medium.com/@smallfishbigsea/a-walk-through-of-alexnet-6cbd137a5637
		//https://medium.com/@RaghavPrabhu/cnn-architectures-lenet-alexnet-vgg-googlenet-and-resnet-7c81c017b848
		//https://neurohive.io/en/popular-networks/vgg16/

		//Tiny ImageNet
		//http://cs231n.stanford.edu/reports/2017/pdfs/930.pdf
		//http://cs231n.stanford.edu/reports/2017/pdfs/931.pdf
		if (net.compare("AlexNet") == 0)
		{
			INPUT_SIZE = 56*56*3;
			LAST_LAYER_SIZE = 200;
			TRAINING_DATA_SIZE = 8;
			TEST_DATA_SIZE = 8;			
		}
		else if (net.compare("VGG16") == 0)
		{
			INPUT_SIZE = 64*64*3;
			LAST_LAYER_SIZE = 200;
			TRAINING_DATA_SIZE = 8;
			TEST_DATA_SIZE = 8;			
		}
		else
			assert(false && "Only AlexNet and VGG16 supported on ImageNet");
	}
	else
		assert(false && "Only MNIST, CIFAR10, and ImageNet supported");


	string filename_train_data_next, filename_train_data_prev;
	string filename_test_data_next, filename_test_data_prev;
	string filename_train_labels_next, filename_train_labels_prev;
	string filename_test_labels_next, filename_test_labels_prev;
	
	// modified to let each party holding a share of data
	if (partyNum == PARTY_A)
	{
		filename_train_data_next = "files/train_data_A";
		filename_train_data_prev = "files/train_data_B";
		filename_test_data_next = "files/test_data_A";
		filename_test_data_prev = "files/test_data_B";
		filename_train_labels_next = "files/train_labels_A";
		filename_train_labels_prev = "files/train_labels_B";
		filename_test_labels_next = "files/test_labels_A";
		filename_test_labels_prev = "files/test_labels_B";
	}
		if (partyNum == PARTY_B)
	{
		filename_train_data_next = "files/train_data_B";
		filename_train_data_prev = "files/train_data_C";
		filename_test_data_next = "files/test_data_B";
		filename_test_data_prev = "files/test_data_C";
		filename_train_labels_next = "files/train_labels_B";
		filename_train_labels_prev = "files/train_labels_C";
		filename_test_labels_next = "files/test_labels_B";
		filename_test_labels_prev = "files/test_labels_C";
	}

	if (partyNum == PARTY_C)
	{
		filename_train_data_next = "files/train_data_C";
		filename_train_data_prev = "files/train_data_A";
		filename_test_data_next = "files/test_data_C";
		filename_test_data_prev = "files/test_data_A";
		filename_train_labels_next = "files/train_labels_C";
		filename_train_labels_prev = "files/train_labels_A";
		filename_test_labels_next = "files/test_labels_C";
		filename_test_labels_prev = "files/test_labels_A";
	}	

	float temp_next = 0, temp_prev = 0;
	ifstream f_next(filename_train_data_next);
	ifstream f_prev(filename_train_data_prev);
	for (int i = 0; i < TRAINING_DATA_SIZE * INPUT_SIZE; ++i)
	{
		f_next >> temp_next; f_prev >> temp_prev;
		trainData.push_back(std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev)));
	}
	f_next.close(); f_prev.close();

	ifstream g_next(filename_train_labels_next);
	ifstream g_prev(filename_train_labels_prev);
	for (int i = 0; i < TRAINING_DATA_SIZE * LAST_LAYER_SIZE; ++i)
	{
		g_next >> temp_next; g_prev >> temp_prev;
		trainLabels.push_back(std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev)));
	}
	g_next.close(); g_prev.close();

	ifstream h_next(filename_test_data_next);
	ifstream h_prev(filename_test_data_prev);
	for (int i = 0; i < TEST_DATA_SIZE * INPUT_SIZE; ++i)
	{
		h_next >> temp_next; h_prev >> temp_prev;
		testData.push_back(std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev)));
	}
	h_next.close(); h_prev.close();

	ifstream k_next(filename_test_labels_next);
	ifstream k_prev(filename_test_labels_prev);
	for (int i = 0; i < TEST_DATA_SIZE * LAST_LAYER_SIZE; ++i)
	{
		k_next >> temp_next; k_prev >> temp_prev;
		testLabels.push_back(std::make_pair(floatToMyType(temp_next), floatToMyType(temp_prev)));
	}
	k_next.close(); k_prev.close();		

	cout << "Loading data done....." << endl;
}
void readMiniBatch(NeuralNetwork* net, string phase)

{
	size_t s = trainData.size();
	size_t t = trainLabels.size();

	// if (phase == "TRAINING")
	// {
	// 	for (int i = 0; i < INPUT_SIZE * MINI_BATCH_SIZE; ++i)
	// 		net->inputData[i] = trainData[(trainDataBatchCounter + i)%s];

	// 	for (int i = 0; i < LAST_LAYER_SIZE * MINI_BATCH_SIZE; ++i)
	// 		net->outputData[i] = trainLabels[(trainLabelsBatchCounter + i)%t];

	// 	trainDataBatchCounter += INPUT_SIZE * MINI_BATCH_SIZE;
	// 	trainLabelsBatchCounter += LAST_LAYER_SIZE * MINI_BATCH_SIZE;
	// }

	// if (trainDataBatchCounter > s)
	// 	trainDataBatchCounter -= s;

	// if (trainLabelsBatchCounter > t)
	// 	trainLabelsBatchCounter -= t;



	size_t p = testData.size();
	size_t q = testLabels.size();

	if (phase == "TESTING")
	{
		for (int i = 0; i < INPUT_SIZE * MINI_BATCH_SIZE; ++i)
			net->inputData[i] = testData[(testDataBatchCounter + i)%p];

		for (int i = 0; i < LAST_LAYER_SIZE * MINI_BATCH_SIZE; ++i)
			net->outputData[i] = testLabels[(testLabelsBatchCounter + i)%q];

		testDataBatchCounter += INPUT_SIZE * MINI_BATCH_SIZE;
		testLabelsBatchCounter += LAST_LAYER_SIZE * MINI_BATCH_SIZE;
	}

	if (testDataBatchCounter > p)
		testDataBatchCounter -= p;

	if (testLabelsBatchCounter > q)
		testLabelsBatchCounter -= q;
}
void printNetwork(NeuralNetwork* net)
{
	for (int i = 0; i < net->layers.size(); ++i)
		net->layers[i]->printLayer();
	cout << "----------------------------------------------" << endl;  	
}
void selectNetwork(string network, string dataset, string security, NeuralNetConfig* config)
{
	assert(((security.compare("Semi-honest") == 0) or (security.compare("Malicious") == 0)) && 
			"Only Semi-honest or Malicious security allowed");
	SECURITY_TYPE = security;
	loadData(network, dataset);

	if (network.compare("SecureML") == 0)
	{
		assert((dataset.compare("MNIST") == 0) && "SecureML only over MNIST");
		NUM_LAYERS = 6;
		WITH_NORMALIZATION = true;
		FCConfig* l0 = new FCConfig(784, MINI_BATCH_SIZE, 128); 
		ReLUConfig* l1 = new ReLUConfig(128, MINI_BATCH_SIZE);
		FCConfig* l2 = new FCConfig(128, MINI_BATCH_SIZE, 128); 
		ReLUConfig* l3 = new ReLUConfig(128, MINI_BATCH_SIZE);
		FCConfig* l4 = new FCConfig(128, MINI_BATCH_SIZE, 10); 
		ReLUConfig* l5 = new ReLUConfig(10, MINI_BATCH_SIZE);
		// BNConfig* l6 = new BNConfig(10, MINI_BATCH_SIZE);
		config->addLayer(l0);
		config->addLayer(l1);
		config->addLayer(l2);
		config->addLayer(l3);
		config->addLayer(l4);
		config->addLayer(l5);
		// config->addLayer(l6);
	}
	else if (network.compare("Sarda") == 0)
	{
		assert((dataset.compare("MNIST") == 0) && "Sarda only over MNIST");
		NUM_LAYERS = 5;
		WITH_NORMALIZATION = true;
		CNNConfig* l0 = new CNNConfig(28,28,1,5,2,2,0,MINI_BATCH_SIZE);
		ReLUConfig* l1 = new ReLUConfig(980, MINI_BATCH_SIZE);
		FCConfig* l2 = new FCConfig(980, MINI_BATCH_SIZE, 100);
		ReLUConfig* l3 = new ReLUConfig(100, MINI_BATCH_SIZE);
		FCConfig* l4 = new FCConfig(100, MINI_BATCH_SIZE, 10);
		config->addLayer(l0);
		config->addLayer(l1);
		config->addLayer(l2);
		config->addLayer(l3);
		config->addLayer(l4);
	}
	else if (network.compare("MiniONN") == 0)
	{
		assert((dataset.compare("MNIST") == 0) && "MiniONN only over MNIST");
		NUM_LAYERS = 10;
		WITH_NORMALIZATION = true;
		CNNConfig* l0 = new CNNConfig(28,28,1,16,5,1,0,MINI_BATCH_SIZE);
		MaxpoolConfig* l1 = new MaxpoolConfig(24,24,16,2,2,MINI_BATCH_SIZE);
		ReLUConfig* l2 = new ReLUConfig(12*12*16, MINI_BATCH_SIZE);
		CNNConfig* l3 = new CNNConfig(12,12,16,16,5,1,0,MINI_BATCH_SIZE);
		MaxpoolConfig* l4 = new MaxpoolConfig(8,8,16,2,2,MINI_BATCH_SIZE);
		ReLUConfig* l5 = new ReLUConfig(4*4*16, MINI_BATCH_SIZE);
		FCConfig* l6 = new FCConfig(4*4*16, MINI_BATCH_SIZE, 100);
		ReLUConfig* l7 = new ReLUConfig(100, MINI_BATCH_SIZE);
		FCConfig* l8 = new FCConfig(100, MINI_BATCH_SIZE, 10);
		ReLUConfig* l9 = new ReLUConfig(10, MINI_BATCH_SIZE);
		config->addLayer(l0);
		config->addLayer(l1);
		config->addLayer(l2);
		config->addLayer(l3);
		config->addLayer(l4);
		config->addLayer(l5);
		config->addLayer(l6);
		config->addLayer(l7);
		config->addLayer(l8);
		config->addLayer(l9);
	}
	else if (network.compare("LeNet") == 0)
	{
		assert((dataset.compare("MNIST") == 0) && "LeNet only over MNIST");
		NUM_LAYERS = 10;
		WITH_NORMALIZATION = true;
		CNNConfig* l0 = new CNNConfig(28,28,1,20,5,1,0,MINI_BATCH_SIZE);
		MaxpoolConfig* l1 = new MaxpoolConfig(24,24,20,2,2,MINI_BATCH_SIZE);
		ReLUConfig* l2 = new ReLUConfig(12*12*20, MINI_BATCH_SIZE);
		CNNConfig* l3 = new CNNConfig(12,12,20,50,5,1,0,MINI_BATCH_SIZE);
		MaxpoolConfig* l4 = new MaxpoolConfig(8,8,50,2,2,MINI_BATCH_SIZE);
		ReLUConfig* l5 = new ReLUConfig(4*4*50, MINI_BATCH_SIZE);
		FCConfig* l6 = new FCConfig(4*4*50, MINI_BATCH_SIZE, 500);
		ReLUConfig* l7 = new ReLUConfig(500, MINI_BATCH_SIZE);
		FCConfig* l8 = new FCConfig(500, MINI_BATCH_SIZE, 10);
		ReLUConfig* l9 = new ReLUConfig(10, MINI_BATCH_SIZE);
		config->addLayer(l0);
		config->addLayer(l1);
		config->addLayer(l2);
		config->addLayer(l3);
		config->addLayer(l4);
		config->addLayer(l5);
		config->addLayer(l6);
		config->addLayer(l7);
		config->addLayer(l8);
		config->addLayer(l9);
	}
	else if (network.compare("AlexNet") == 0)//--ljf
	{
		if(dataset.compare("MNIST") == 0)
			assert(false && "No AlexNet on MNIST");
		else if (dataset.compare("DIAMOND") == 0)
		{
			cout<<"DIAMOND network loading"<<endl;
			NUM_LAYERS = 21;
			WITH_NORMALIZATION = false;
			// CNNConfig* l0 = new CNNConfig(65,65,3,48,11,1,0,MINI_BATCH_SIZE);//input: 65 * 65 , output: 55 * 55 
			// ReLUConfig* l1 = new ReLUConfig(55*55*48,MINI_BATCH_SIZE);// output: 55 * 55 	
			// MaxpoolConfig* l2 = new MaxpoolConfig(55,55,48,3,2,MINI_BATCH_SIZE);//input 55 * 55, output   (55-3)/2+1 = 27
			// BNConfig * l3 = new BNConfig(27*27*48,MINI_BATCH_SIZE);//output: 27 *27

			// CNNConfig* l4 = new CNNConfig(27,27,48,128,5,1,2,MINI_BATCH_SIZE);//input: 27 *27 , output: 12 *12 （31 -5）/1+1 = 27
			// ReLUConfig* l5 = new ReLUConfig(27*27*128,MINI_BATCH_SIZE);		// output: 27 * 27
			// MaxpoolConfig* l6 = new MaxpoolConfig(27,27,128,3,2,MINI_BATCH_SIZE);// output: 13 *13  (27 - 3)/2 +1 = 1
			// BNConfig * l7 = new BNConfig(13*13*128,MINI_BATCH_SIZE);//output: 13 *13
			// // CNNConfig* l8 = new CNNConfig(13,13,128,192,3,1,1,MINI_BATCH_SIZE);// output: 13 *13 (13 +2 -3)/1 =1 =13
			// // ReLUConfig* l9 = new ReLUConfig(13*13*192,MINI_BATCH_SIZE);// output: 13 *13
			// // CNNConfig* l10 = new CNNConfig(13,13,192,192,3,1,1,MINI_BATCH_SIZE);// output: 13 *13
			// // ReLUConfig* l11 = new ReLUConfig(13*13*192,MINI_BATCH_SIZE);//output: 13 *13
			// // CNNConfig* l12 = new CNNConfig(13,13,192,128,3,1,1,MINI_BATCH_SIZE);//output: 13 *13
			// // ReLUConfig* l13 = new ReLUConfig(13*13*128,MINI_BATCH_SIZE);//output: 13 *13
			// // MaxpoolConfig* l14 = new MaxpoolConfig(13,13,128,3,2,MINI_BATCH_SIZE);//output: 6*6 （13-3）/2+1  = 6

			// // FCConfig* l15 = new FCConfig(4608,MINI_BATCH_SIZE,640);
			// // ReLUConfig* l16 = new ReLUConfig(640,MINI_BATCH_SIZE);
			// // FCConfig* l17 = new FCConfig(640,MINI_BATCH_SIZE,640);
			// // ReLUConfig* l18 = new ReLUConfig(640,MINI_BATCH_SIZE);
			// // FCConfig* l19 = new FCConfig(1024,MINI_BATCH_SIZE,2);

			// CNNConfig* l8 = new CNNConfig(13,13,128,192,3,1,1,MINI_BATCH_SIZE);// output: 13 *13 (13 +2 -3)/1 + 1 =13
			// ReLUConfig* l9 = new ReLUConfig(13*13*192,MINI_BATCH_SIZE);// output: 13 * 13
			// CNNConfig* l10 = new CNNConfig(13,13,192,192,3,1,1,MINI_BATCH_SIZE);// output: 13 *13
			// ReLUConfig* l11 = new ReLUConfig(13*13*192,MINI_BATCH_SIZE);//output: 13 * 13

			// //justify from here.
			// CNNConfig* l12 = new CNNConfig(13,13,192,128,3,2,1,MINI_BATCH_SIZE);//output: 13 *13 (13 +2 -3)/2 + 1 =7 
			// ReLUConfig* l13 = new ReLUConfig(7*7*128,MINI_BATCH_SIZE);//output: 7 * 7
			// MaxpoolConfig* l14 = new MaxpoolConfig(7,7,128,3,2,MINI_BATCH_SIZE);//output: 3*3 (7-3）/2+1  = 3

			// FCConfig* l15 = new FCConfig(1152,MINI_BATCH_SIZE,1024);
			// ReLUConfig* l16 = new ReLUConfig(1024,MINI_BATCH_SIZE);
			// FCConfig* l17 = new FCConfig(1024,MINI_BATCH_SIZE,1024);
			// ReLUConfig* l18 = new ReLUConfig(1024,MINI_BATCH_SIZE);
			// FCConfig* l19 = new FCConfig(1024,MINI_BATCH_SIZE,2);
			// ReLUConfig* l20 = new ReLUConfig(2,MINI_BATCH_SIZE);
			// config->addLayer(l0);
			// config->addLayer(l1);
			// config->addLayer(l2);
			// config->addLayer(l3);
			// config->addLayer(l4);
			// config->addLayer(l5);
			// config->addLayer(l6);
			// config->addLayer(l7);
			// config->addLayer(l8);
			// config->addLayer(l9);
			// config->addLayer(l10);
			// config->addLayer(l11);
			// config->addLayer(l12);
			// config->addLayer(l13);
			// config->addLayer(l14);
			// config->addLayer(l15);
			// config->addLayer(l16);
			// config->addLayer(l17);
			// config->addLayer(l18);
			// config->addLayer(l19);
			// config->addLayer(l20);
			// cout<<"DIAMOND network loading finished"<<endl;

			CNNConfig* l0 = new CNNConfig(227,227,3,96,11,4,0,MINI_BATCH_SIZE);
			ReLUConfig* l1 = new ReLUConfig(55*55*96,MINI_BATCH_SIZE);
			MaxpoolConfig* l2 = new MaxpoolConfig(55,55,96,3,2,MINI_BATCH_SIZE);
			BNConfig* l3 = new BNConfig(27*27*96,MINI_BATCH_SIZE);

			CNNConfig* l4 = new CNNConfig(27,27,96,256,5,1,2,MINI_BATCH_SIZE);
			ReLUConfig* l5 = new ReLUConfig(27*27*256,MINI_BATCH_SIZE);	
			MaxpoolConfig* l6 = new MaxpoolConfig(27,27,256,3,2,MINI_BATCH_SIZE);
	
			BNConfig* l7 = new BNConfig(13*13*256,MINI_BATCH_SIZE);
 			CNNConfig* l8 = new CNNConfig(13,13,256,384,3,1,1,MINI_BATCH_SIZE);
			ReLUConfig* l9 = new ReLUConfig(13*13*384,MINI_BATCH_SIZE);
			CNNConfig* l10 = new CNNConfig(13,13,384,384,3,1,1,MINI_BATCH_SIZE);

			ReLUConfig* l11 = new ReLUConfig(13*13*384,MINI_BATCH_SIZE);
			CNNConfig* l12 = new CNNConfig(13,13,384,256,3,1,1,MINI_BATCH_SIZE);
			ReLUConfig* l13 = new ReLUConfig(13*13*256,MINI_BATCH_SIZE);			
			MaxpoolConfig* l14 = new MaxpoolConfig(13,13,256,3,2,MINI_BATCH_SIZE);

			FCConfig* l15 = new FCConfig(6*6*256,MINI_BATCH_SIZE,4096);
			ReLUConfig* l16 = new ReLUConfig(4096,MINI_BATCH_SIZE);
			FCConfig* l17 = new FCConfig(4096,MINI_BATCH_SIZE,4096);
			ReLUConfig* l18 = new ReLUConfig(4096,MINI_BATCH_SIZE);
			FCConfig* l19 = new FCConfig(4096,MINI_BATCH_SIZE,2);
			ReLUConfig* l20 = new ReLUConfig(2,MINI_BATCH_SIZE);
			config->addLayer(l0);
			config->addLayer(l1);
			config->addLayer(l2);
			config->addLayer(l3);
			config->addLayer(l4);
			config->addLayer(l5);
			config->addLayer(l6);
			config->addLayer(l7);
			config->addLayer(l8);
			config->addLayer(l9);
			config->addLayer(l10);
			config->addLayer(l11);
			config->addLayer(l12);
			config->addLayer(l13);
			config->addLayer(l14);
			config->addLayer(l15);
			config->addLayer(l16);
			config->addLayer(l17);
			config->addLayer(l18);
			config->addLayer(l19);
			config->addLayer(l20);
			cout<<"DIAMOND network loading finished"<<endl;

		}
		else if (dataset.compare("CIFAR10") == 0)
		{
			NUM_LAYERS = 20;
			// NUM_LAYERS = 18;		//Without BN
			WITH_NORMALIZATION = false;
			CNNConfig* l0 = new CNNConfig(33,33,3,96,11,4,9,MINI_BATCH_SIZE);//output: 11*11 (33+2*9 - 11)/4+1=11
			MaxpoolConfig* l1 = new MaxpoolConfig(11,11,96,3,2,MINI_BATCH_SIZE);//output : (11 - 3)/2 + 1= 5
			ReLUConfig* l2 = new ReLUConfig(5*5*96,MINI_BATCH_SIZE);//output : 5*5
			BNConfig * l3 = new BNConfig(5*5*96,MINI_BATCH_SIZE);//out 5*5

			CNNConfig* l4 = new CNNConfig(5,5,96,256,5,1,1,MINI_BATCH_SIZE);//out:3*3  (5+1*2-5)/1 + 1 =3
			MaxpoolConfig* l5 = new MaxpoolConfig(3,3,256,3,2,MINI_BATCH_SIZE);//out: 1*1 (3-3)/2 +1 =1 
			ReLUConfig* l6 = new ReLUConfig(1*1*256,MINI_BATCH_SIZE);		//out 1*1
			BNConfig * l7 = new BNConfig(1*1*256,MINI_BATCH_SIZE);//out 1*1

			CNNConfig* l8 = new CNNConfig(1,1,256,384,3,1,1,MINI_BATCH_SIZE);
			ReLUConfig* l9 = new ReLUConfig(1*1*384,MINI_BATCH_SIZE);
			CNNConfig* l10 = new CNNConfig(1,1,384,384,3,1,1,MINI_BATCH_SIZE);
			ReLUConfig* l11 = new ReLUConfig(1*1*384,MINI_BATCH_SIZE);
			CNNConfig* l12 = new CNNConfig(1,1,384,256,3,1,1,MINI_BATCH_SIZE);
			ReLUConfig* l13 = new ReLUConfig(1*1*256,MINI_BATCH_SIZE);

			FCConfig* l14 = new FCConfig(1*1*256,MINI_BATCH_SIZE,256);
			ReLUConfig* l15 = new ReLUConfig(256,MINI_BATCH_SIZE);
			FCConfig* l16 = new FCConfig(256,MINI_BATCH_SIZE,256);
			ReLUConfig* l17 = new ReLUConfig(256,MINI_BATCH_SIZE);
			FCConfig* l18 = new FCConfig(256,MINI_BATCH_SIZE,10);
			ReLUConfig* l19 = new ReLUConfig(10,MINI_BATCH_SIZE);
			config->addLayer(l0);
			config->addLayer(l1);
			config->addLayer(l2);
			config->addLayer(l3);
			config->addLayer(l4);
			config->addLayer(l5);
			config->addLayer(l6);
			config->addLayer(l7);
			config->addLayer(l8);
			config->addLayer(l9);
			config->addLayer(l10);
			config->addLayer(l11);
			config->addLayer(l12);
			config->addLayer(l13);
			config->addLayer(l14);
			config->addLayer(l15);
			config->addLayer(l16);
			config->addLayer(l17);
			config->addLayer(l18);
			config->addLayer(l19);
		}
		else if (dataset.compare("ImageNet") == 0)
		{
			NUM_LAYERS = 19;
			// NUM_LAYERS = 17;		//Without BN
			WITH_NORMALIZATION = false;
			CNNConfig* l0 = new CNNConfig(56,56,3,64,7,1,3,MINI_BATCH_SIZE);
			CNNConfig* l1 = new CNNConfig(56,56,64,64,5,1,2,MINI_BATCH_SIZE);
			MaxpoolConfig* l2 = new MaxpoolConfig(56,56,64,2,2,MINI_BATCH_SIZE);
			ReLUConfig* l3 = new ReLUConfig(28*28*64,MINI_BATCH_SIZE);		
			BNConfig * l4 = new BNConfig(28*28*64,MINI_BATCH_SIZE);

			CNNConfig* l5 = new CNNConfig(28,28,64,128,5,1,2,MINI_BATCH_SIZE);
			MaxpoolConfig* l6 = new MaxpoolConfig(28,28,128,2,2,MINI_BATCH_SIZE);
			ReLUConfig* l7 = new ReLUConfig(14*14*128,MINI_BATCH_SIZE);		
			BNConfig * l8 = new BNConfig(14*14*128,MINI_BATCH_SIZE);

			CNNConfig* l9 = new CNNConfig(14,14,128,256,3,1,1,MINI_BATCH_SIZE);
			CNNConfig* l10 = new CNNConfig(14,14,256,256,3,1,1,MINI_BATCH_SIZE);
			MaxpoolConfig* l11 = new MaxpoolConfig(14,14,256,2,2,MINI_BATCH_SIZE);
			ReLUConfig* l12 = new ReLUConfig(7*7*256,MINI_BATCH_SIZE);

			FCConfig* l13 = new FCConfig(7*7*256,MINI_BATCH_SIZE,1024);
			ReLUConfig* l14 = new ReLUConfig(1024,MINI_BATCH_SIZE);
			FCConfig* l15 = new FCConfig(1024,MINI_BATCH_SIZE,1024);
			ReLUConfig* l16 = new ReLUConfig(1024,MINI_BATCH_SIZE);
			FCConfig* l17 = new FCConfig(1024,MINI_BATCH_SIZE,200);
			ReLUConfig* l18 = new ReLUConfig(200,MINI_BATCH_SIZE);
			config->addLayer(l0);
			config->addLayer(l1);
			config->addLayer(l2);
			config->addLayer(l3);
			config->addLayer(l4);
			config->addLayer(l5);
			config->addLayer(l6);
			config->addLayer(l7);
			config->addLayer(l8);
			config->addLayer(l9);
			config->addLayer(l10);
			config->addLayer(l11);
			config->addLayer(l12);
			config->addLayer(l13);
			config->addLayer(l14);
			config->addLayer(l15);
			config->addLayer(l16);
			config->addLayer(l17);
			config->addLayer(l18);
		}
	}
	else if (network.compare("VGG16") == 0)
	{
		if(dataset.compare("MNIST") == 0)
			assert(false && "No VGG16 on MNIST");
		else if (dataset.compare("CIFAR10") == 0)
		{
			NUM_LAYERS = 37;
			WITH_NORMALIZATION = false;
			CNNConfig* l0 = new CNNConfig(32,32,3,64,3,1,1,MINI_BATCH_SIZE);
			ReLUConfig* l1 = new ReLUConfig(32*32*64,MINI_BATCH_SIZE);		
			CNNConfig* l2 = new CNNConfig(32,32,64,64,3,1,1,MINI_BATCH_SIZE);
			MaxpoolConfig* l3 = new MaxpoolConfig(32,32,64,2,2,MINI_BATCH_SIZE);
			ReLUConfig* l4 = new ReLUConfig(16*16*64,MINI_BATCH_SIZE);

			CNNConfig* l5 = new CNNConfig(16,16,64,128,3,1,1,MINI_BATCH_SIZE);
			ReLUConfig* l6 = new ReLUConfig(16*16*128,MINI_BATCH_SIZE);
			CNNConfig* l7 = new CNNConfig(16,16,128,128,3,1,1,MINI_BATCH_SIZE);
			MaxpoolConfig* l8 = new MaxpoolConfig(16,16,128,2,2,MINI_BATCH_SIZE);
			ReLUConfig* l9 = new ReLUConfig(8*8*128,MINI_BATCH_SIZE);

			CNNConfig* l10 = new CNNConfig(8,8,128,256,3,1,1,MINI_BATCH_SIZE);
			ReLUConfig* l11 = new ReLUConfig(8*8*256,MINI_BATCH_SIZE);
			CNNConfig* l12 = new CNNConfig(8,8,256,256,3,1,1,MINI_BATCH_SIZE);
			ReLUConfig* l13 = new ReLUConfig(8*8*256,MINI_BATCH_SIZE);
			CNNConfig* l14 = new CNNConfig(8,8,256,256,3,1,1,MINI_BATCH_SIZE);
			MaxpoolConfig* l15 = new MaxpoolConfig(8,8,256,2,2,MINI_BATCH_SIZE);
			ReLUConfig* l16 = new ReLUConfig(4*4*256,MINI_BATCH_SIZE);

			CNNConfig* l17 = new CNNConfig(4,4,256,512,3,1,1,MINI_BATCH_SIZE);
			ReLUConfig* l18 = new ReLUConfig(4*4*512,MINI_BATCH_SIZE);
			CNNConfig* l19 = new CNNConfig(4,4,512,512,3,1,1,MINI_BATCH_SIZE);
			ReLUConfig* l20 = new ReLUConfig(4*4*512,MINI_BATCH_SIZE);
			CNNConfig* l21 = new CNNConfig(4,4,512,512,3,1,1,MINI_BATCH_SIZE);
			MaxpoolConfig* l22 = new MaxpoolConfig(4,4,512,2,2,MINI_BATCH_SIZE);
			ReLUConfig* l23 = new ReLUConfig(2*2*512,MINI_BATCH_SIZE);

			CNNConfig* l24 = new CNNConfig(2,2,512,512,3,1,1,MINI_BATCH_SIZE);
			ReLUConfig* l25 = new ReLUConfig(2*2*512,MINI_BATCH_SIZE);
			CNNConfig* l26 = new CNNConfig(2,2,512,512,3,1,1,MINI_BATCH_SIZE);
			ReLUConfig* l27 = new ReLUConfig(2*2*512,MINI_BATCH_SIZE);
			CNNConfig* l28 = new CNNConfig(2,2,512,512,3,1,1,MINI_BATCH_SIZE);
			MaxpoolConfig* l29 = new MaxpoolConfig(2,2,512,2,2,MINI_BATCH_SIZE);
			ReLUConfig* l30 = new ReLUConfig(1*1*512,MINI_BATCH_SIZE);

			FCConfig* l31 = new FCConfig(1*1*512,MINI_BATCH_SIZE,4096);
			ReLUConfig* l32 = new ReLUConfig(4096,MINI_BATCH_SIZE);
			FCConfig* l33 = new FCConfig(4096, MINI_BATCH_SIZE, 4096);
			ReLUConfig* l34 = new ReLUConfig(4096, MINI_BATCH_SIZE);
			FCConfig* l35 = new FCConfig(4096, MINI_BATCH_SIZE, 1000);
			ReLUConfig* l36 = new ReLUConfig(1000, MINI_BATCH_SIZE);
			config->addLayer(l0);
			config->addLayer(l1);
			config->addLayer(l2);
			config->addLayer(l3);
			config->addLayer(l4);
			config->addLayer(l5);
			config->addLayer(l6);
			config->addLayer(l7);
			config->addLayer(l8);
			config->addLayer(l9);
			config->addLayer(l10);
			config->addLayer(l11);
			config->addLayer(l12);
			config->addLayer(l13);
			config->addLayer(l14);
			config->addLayer(l15);
			config->addLayer(l16);
			config->addLayer(l17);
			config->addLayer(l18);
			config->addLayer(l19);
			config->addLayer(l20);
			config->addLayer(l21);
			config->addLayer(l22);
			config->addLayer(l23);
			config->addLayer(l24);
			config->addLayer(l25);
			config->addLayer(l26);
			config->addLayer(l27);
			config->addLayer(l28);
			config->addLayer(l29);
			config->addLayer(l30);
			config->addLayer(l31);
			config->addLayer(l32);
			config->addLayer(l33);
			config->addLayer(l34);
			config->addLayer(l35);
			config->addLayer(l36);
		}
		else if (dataset.compare("ImageNet") == 0)
		{
			NUM_LAYERS = 37;
			WITH_NORMALIZATION = false;
			CNNConfig* l0 = new CNNConfig(64,64,3,64,3,1,1,MINI_BATCH_SIZE);
			ReLUConfig* l1 = new ReLUConfig(64*64*64,MINI_BATCH_SIZE);		
			CNNConfig* l2 = new CNNConfig(64,64,64,64,3,1,1,MINI_BATCH_SIZE);
			MaxpoolConfig* l3 = new MaxpoolConfig(64,64,64,2,2,MINI_BATCH_SIZE);
			ReLUConfig* l4 = new ReLUConfig(32*32*64,MINI_BATCH_SIZE);

			CNNConfig* l5 = new CNNConfig(32,32,64,128,3,1,1,MINI_BATCH_SIZE);
			ReLUConfig* l6 = new ReLUConfig(32*32*128,MINI_BATCH_SIZE);
			CNNConfig* l7 = new CNNConfig(32,32,128,128,3,1,1,MINI_BATCH_SIZE);
			MaxpoolConfig* l8 = new MaxpoolConfig(32,32,128,2,2,MINI_BATCH_SIZE);
			ReLUConfig* l9 = new ReLUConfig(16*16*128,MINI_BATCH_SIZE);

			CNNConfig* l10 = new CNNConfig(16,16,128,256,3,1,1,MINI_BATCH_SIZE);
			ReLUConfig* l11 = new ReLUConfig(16*16*256,MINI_BATCH_SIZE);
			CNNConfig* l12 = new CNNConfig(16,16,256,256,3,1,1,MINI_BATCH_SIZE);
			ReLUConfig* l13 = new ReLUConfig(16*16*256,MINI_BATCH_SIZE);
			CNNConfig* l14 = new CNNConfig(16,16,256,256,3,1,1,MINI_BATCH_SIZE);
			MaxpoolConfig* l15 = new MaxpoolConfig(16,16,256,2,2,MINI_BATCH_SIZE);
			ReLUConfig* l16 = new ReLUConfig(8*8*256,MINI_BATCH_SIZE);

			CNNConfig* l17 = new CNNConfig(8,8,256,512,3,1,1,MINI_BATCH_SIZE);
			ReLUConfig* l18 = new ReLUConfig(8*8*512,MINI_BATCH_SIZE);
			CNNConfig* l19 = new CNNConfig(8,8,512,512,3,1,1,MINI_BATCH_SIZE);
			ReLUConfig* l20 = new ReLUConfig(8*8*512,MINI_BATCH_SIZE);
			CNNConfig* l21 = new CNNConfig(8,8,512,512,3,1,1,MINI_BATCH_SIZE);
			MaxpoolConfig* l22 = new MaxpoolConfig(8,8,512,2,2,MINI_BATCH_SIZE);
			ReLUConfig* l23 = new ReLUConfig(4*4*512,MINI_BATCH_SIZE);

			CNNConfig* l24 = new CNNConfig(4,4,512,512,3,1,1,MINI_BATCH_SIZE);
			ReLUConfig* l25 = new ReLUConfig(4*4*512,MINI_BATCH_SIZE);
			CNNConfig* l26 = new CNNConfig(4,4,512,512,3,1,1,MINI_BATCH_SIZE);
			ReLUConfig* l27 = new ReLUConfig(4*4*512,MINI_BATCH_SIZE);
			CNNConfig* l28 = new CNNConfig(4,4,512,512,3,1,1,MINI_BATCH_SIZE);
			MaxpoolConfig* l29 = new MaxpoolConfig(4,4,512,2,2,MINI_BATCH_SIZE);
			ReLUConfig* l30 = new ReLUConfig(2*2*512,MINI_BATCH_SIZE);

			FCConfig* l31 = new FCConfig(2*2*512,MINI_BATCH_SIZE,2048);
			ReLUConfig* l32 = new ReLUConfig(2048,MINI_BATCH_SIZE);
			FCConfig* l33 = new FCConfig(2048, MINI_BATCH_SIZE, 2048);
			ReLUConfig* l34 = new ReLUConfig(2048, MINI_BATCH_SIZE);
			FCConfig* l35 = new FCConfig(2048, MINI_BATCH_SIZE, 200);
			ReLUConfig* l36 = new ReLUConfig(200, MINI_BATCH_SIZE);
			config->addLayer(l0);
			config->addLayer(l1);
			config->addLayer(l2);
			config->addLayer(l3);
			config->addLayer(l4);
			config->addLayer(l5);
			config->addLayer(l6);
			config->addLayer(l7);
			config->addLayer(l8);
			config->addLayer(l9);
			config->addLayer(l10);
			config->addLayer(l11);
			config->addLayer(l12);
			config->addLayer(l13);
			config->addLayer(l14);
			config->addLayer(l15);
			config->addLayer(l16);
			config->addLayer(l17);
			config->addLayer(l18);
			config->addLayer(l19);
			config->addLayer(l20);
			config->addLayer(l21);
			config->addLayer(l22);
			config->addLayer(l23);
			config->addLayer(l24);
			config->addLayer(l25);
			config->addLayer(l26);
			config->addLayer(l27);
			config->addLayer(l28);
			config->addLayer(l29);
			config->addLayer(l30);
			config->addLayer(l31);
			config->addLayer(l32);
			config->addLayer(l33);
			config->addLayer(l34);
			config->addLayer(l35);
			config->addLayer(l36);
		}
		else if (dataset.compare("DIAMOND") == 0)//--ljf
		{
			NUM_LAYERS = 37;
			WITH_NORMALIZATION = false;
			CNNConfig* l0 = new CNNConfig(224,224,3,64,3,1,1,MINI_BATCH_SIZE);
			ReLUConfig* l1 = new ReLUConfig(224*224*64,MINI_BATCH_SIZE);		
			CNNConfig* l2 = new CNNConfig(224,224,64,64,3,1,1,MINI_BATCH_SIZE);
			MaxpoolConfig* l3 = new MaxpoolConfig(224,224,64,2,2,MINI_BATCH_SIZE);
			ReLUConfig* l4 = new ReLUConfig(112*112*64,MINI_BATCH_SIZE);

			CNNConfig* l5 = new CNNConfig(112,112,64,128,3,1,1,MINI_BATCH_SIZE);
			ReLUConfig* l6 = new ReLUConfig(112*112*128,MINI_BATCH_SIZE);
			CNNConfig* l7 = new CNNConfig(112,112,128,128,3,1,1,MINI_BATCH_SIZE);
			MaxpoolConfig* l8 = new MaxpoolConfig(112,112,128,2,2,MINI_BATCH_SIZE);
			ReLUConfig* l9 = new ReLUConfig(56*56*128,MINI_BATCH_SIZE);

			CNNConfig* l10 = new CNNConfig(56,56,128,256,3,1,1,MINI_BATCH_SIZE);
			ReLUConfig* l11 = new ReLUConfig(56*56*256,MINI_BATCH_SIZE);
			CNNConfig* l12 = new CNNConfig(56,56,256,256,3,1,1,MINI_BATCH_SIZE);
			ReLUConfig* l13 = new ReLUConfig(56*56*256,MINI_BATCH_SIZE);
			CNNConfig* l14 = new CNNConfig(56,56,256,256,3,1,1,MINI_BATCH_SIZE);
			MaxpoolConfig* l15 = new MaxpoolConfig(56,56,256,2,2,MINI_BATCH_SIZE);
			ReLUConfig* l16 = new ReLUConfig(28*28*256,MINI_BATCH_SIZE);

			CNNConfig* l17 = new CNNConfig(28,28,256,512,3,1,1,MINI_BATCH_SIZE);
			ReLUConfig* l18 = new ReLUConfig(28*28*512,MINI_BATCH_SIZE);
			CNNConfig* l19 = new CNNConfig(28,28,512,512,3,1,1,MINI_BATCH_SIZE);
			ReLUConfig* l20 = new ReLUConfig(28*28*512,MINI_BATCH_SIZE);
			CNNConfig* l21 = new CNNConfig(28,28,512,512,3,1,1,MINI_BATCH_SIZE);
			MaxpoolConfig* l22 = new MaxpoolConfig(28,28,512,2,2,MINI_BATCH_SIZE);
			ReLUConfig* l23 = new ReLUConfig(14*14*512,MINI_BATCH_SIZE);

			CNNConfig* l24 = new CNNConfig(14,14,512,512,3,1,1,MINI_BATCH_SIZE);
			ReLUConfig* l25 = new ReLUConfig(14*14*512,MINI_BATCH_SIZE);
			CNNConfig* l26 = new CNNConfig(14,14,512,512,3,1,1,MINI_BATCH_SIZE);
			ReLUConfig* l27 = new ReLUConfig(14*14*512,MINI_BATCH_SIZE);
			CNNConfig* l28 = new CNNConfig(14,14,512,512,3,1,1,MINI_BATCH_SIZE);
			MaxpoolConfig* l29 = new MaxpoolConfig(14,14,512,2,2,MINI_BATCH_SIZE);
			ReLUConfig* l30 = new ReLUConfig(7*7*512,MINI_BATCH_SIZE);

			FCConfig* l31 = new FCConfig(7*7*512,MINI_BATCH_SIZE,4096);
			ReLUConfig* l32 = new ReLUConfig(4096,MINI_BATCH_SIZE);
			FCConfig* l33 = new FCConfig(4096, MINI_BATCH_SIZE, 4096);
			ReLUConfig* l34 = new ReLUConfig(4096, MINI_BATCH_SIZE);
			FCConfig* l35 = new FCConfig(4096, MINI_BATCH_SIZE, 2);
			ReLUConfig* l36 = new ReLUConfig(2, MINI_BATCH_SIZE);
			config->addLayer(l0);
			config->addLayer(l1);
			config->addLayer(l2);
			config->addLayer(l3);
			config->addLayer(l4);
			config->addLayer(l5);
			config->addLayer(l6);
			config->addLayer(l7);
			config->addLayer(l8);
			config->addLayer(l9);
			config->addLayer(l10);
			config->addLayer(l11);
			config->addLayer(l12);
			config->addLayer(l13);
			config->addLayer(l14);
			config->addLayer(l15);
			config->addLayer(l16);
			config->addLayer(l17);
			config->addLayer(l18);
			config->addLayer(l19);
			config->addLayer(l20);
			config->addLayer(l21);
			config->addLayer(l22);
			config->addLayer(l23);
			config->addLayer(l24);
			config->addLayer(l25);
			config->addLayer(l26);
			config->addLayer(l27);
			config->addLayer(l28);
			config->addLayer(l29);
			config->addLayer(l30);
			config->addLayer(l31);
			config->addLayer(l32);
			config->addLayer(l33);
			config->addLayer(l34);
			config->addLayer(l35);
			config->addLayer(l36);
		}
	}
	else if (network.compare("RepVgg") == 0)//--ljf
	{
		if(dataset.compare("MNIST") == 0 || dataset.compare("CIFAR10") == 0 || dataset.compare("ImageNet") == 0 )
			assert(false && "No RepVgg on MNIST,CIFAR10, ImageNet");
		else if(dataset.compare("DIAMOND") == 0){
		NUM_LAYERS = 47;
		WITH_NORMALIZATION = false;
		//stage 0 
		CNNConfig* l0 = new CNNConfig(225,225,3,48,3,2,1,MINI_BATCH_SIZE);
		ReLUConfig* l1 = new ReLUConfig(113*113*48,MINI_BATCH_SIZE);		

		//stage1
		CNNConfig* l2 = new CNNConfig(113,113,48,48,3,2,1,MINI_BATCH_SIZE);
		ReLUConfig* l3 = new ReLUConfig(57*57*48,MINI_BATCH_SIZE);
		CNNConfig* l4 = new CNNConfig(57,57,48,48,3,1,1,MINI_BATCH_SIZE);
		ReLUConfig* l5 = new ReLUConfig(57*57*48,MINI_BATCH_SIZE);

		//stage2
		CNNConfig* l6 = new CNNConfig(57,57,48,96,3,2,1,MINI_BATCH_SIZE);
		ReLUConfig* l7 = new ReLUConfig(29*29*96,MINI_BATCH_SIZE);
		CNNConfig* l8 = new CNNConfig(29,29,96,96,3,1,1,MINI_BATCH_SIZE);
		ReLUConfig* l9 = new ReLUConfig(29*29*96,MINI_BATCH_SIZE);
		CNNConfig* l10 = new CNNConfig(29,29,96,96,3,1,1,MINI_BATCH_SIZE);
		ReLUConfig* l11 = new ReLUConfig(29*29*96,MINI_BATCH_SIZE);
		CNNConfig* l12 = new CNNConfig(29,29,96,96,3,1,1,MINI_BATCH_SIZE);
		ReLUConfig* l13 = new ReLUConfig(29*29*96,MINI_BATCH_SIZE);

		//stage3
		CNNConfig* l14 = new CNNConfig(29,29,96,192,3,2,1,MINI_BATCH_SIZE);
		ReLUConfig* l15 = new ReLUConfig(15*15*192,MINI_BATCH_SIZE);
		CNNConfig* l16 = new CNNConfig(15,15,192,192,3,1,1,MINI_BATCH_SIZE);
		ReLUConfig* l17 = new ReLUConfig(15*15*192,MINI_BATCH_SIZE);
		CNNConfig* l18 = new CNNConfig(15,15,192,192,3,1,1,MINI_BATCH_SIZE);
		ReLUConfig* l19 = new ReLUConfig(15*15*192,MINI_BATCH_SIZE);
		CNNConfig* l20 = new CNNConfig(15,15,192,192,3,1,1,MINI_BATCH_SIZE);
		ReLUConfig* l21 = new ReLUConfig(15*15*192,MINI_BATCH_SIZE);
		CNNConfig* l22 = new CNNConfig(15,15,192,192,3,1,1,MINI_BATCH_SIZE);
		ReLUConfig* l23 = new ReLUConfig(15*15*192,MINI_BATCH_SIZE);
		CNNConfig* l24 = new CNNConfig(15,15,192,192,3,1,1,MINI_BATCH_SIZE);
		ReLUConfig* l25 = new ReLUConfig(15*15*192,MINI_BATCH_SIZE);
		CNNConfig* l26 = new CNNConfig(15,15,192,192,3,1,1,MINI_BATCH_SIZE);
		ReLUConfig* l27 = new ReLUConfig(15*15*192,MINI_BATCH_SIZE);
		CNNConfig* l28 = new CNNConfig(15,15,192,192,3,1,1,MINI_BATCH_SIZE);
		ReLUConfig* l29 = new ReLUConfig(15*15*192,MINI_BATCH_SIZE);
		CNNConfig* l30 = new CNNConfig(15,15,192,192,3,1,1,MINI_BATCH_SIZE);
		ReLUConfig* l31 = new ReLUConfig(15*15*192,MINI_BATCH_SIZE);
		CNNConfig* l32 = new CNNConfig(15,15,192,192,3,1,1,MINI_BATCH_SIZE);
		ReLUConfig* l33 = new ReLUConfig(15*15*192,MINI_BATCH_SIZE);
		CNNConfig* l34 = new CNNConfig(15,15,192,192,3,1,1,MINI_BATCH_SIZE);
		ReLUConfig* l35 = new ReLUConfig(15*15*192,MINI_BATCH_SIZE);
		CNNConfig* l36 = new CNNConfig(15,15,192,192,3,1,1,MINI_BATCH_SIZE);
		ReLUConfig* l37 = new ReLUConfig(15*15*192,MINI_BATCH_SIZE);
		CNNConfig* l38 = new CNNConfig(15,15,192,192,3,1,1,MINI_BATCH_SIZE);
		ReLUConfig* l39 = new ReLUConfig(15*15*192,MINI_BATCH_SIZE);
		CNNConfig* l40 = new CNNConfig(15,15,192,192,3,1,1,MINI_BATCH_SIZE);
		ReLUConfig* l41 = new ReLUConfig(15*15*192,MINI_BATCH_SIZE);

		CNNConfig* l42 = new CNNConfig(15,15,192,1024,3,2,1,MINI_BATCH_SIZE);
		ReLUConfig* l43 = new ReLUConfig(8*8*1024,MINI_BATCH_SIZE);
		AdaptAvgpoolLayerConfig* l44 = new AdaptAvgpoolLayerConfig(8,8,1024,8,1,MINI_BATCH_SIZE);//stride can not be 0
		FCConfig* l45 = new FCConfig(1024, MINI_BATCH_SIZE, 2);
		ReLUConfig* l46 = new ReLUConfig(2,MINI_BATCH_SIZE);
		

		config->addLayer(l0);
		config->addLayer(l1);
		config->addLayer(l2);
		config->addLayer(l3);
		config->addLayer(l4);
		config->addLayer(l5);
		config->addLayer(l6);
		config->addLayer(l7);
		config->addLayer(l8);
		config->addLayer(l9);
		config->addLayer(l10);
		config->addLayer(l11);
		config->addLayer(l12);
		config->addLayer(l13);
		config->addLayer(l14);
		config->addLayer(l15);
		config->addLayer(l16);
		config->addLayer(l17);
		config->addLayer(l18);
		config->addLayer(l19);
		config->addLayer(l20);
		config->addLayer(l21);
		config->addLayer(l22);
		config->addLayer(l23);
		config->addLayer(l24);
		config->addLayer(l25);
		config->addLayer(l26);
		config->addLayer(l27);
		config->addLayer(l28);
		config->addLayer(l29);
		config->addLayer(l30);
		config->addLayer(l31);
		config->addLayer(l32);
		config->addLayer(l33);
		config->addLayer(l34);
		config->addLayer(l35);
		config->addLayer(l36);
		config->addLayer(l37);
		config->addLayer(l38);
		config->addLayer(l39);
		config->addLayer(l40);
		config->addLayer(l41);
		config->addLayer(l42);
		config->addLayer(l43);
		config->addLayer(l44);
		config->addLayer(l45);
		config->addLayer(l46);
		}
	}
	else
		assert(false && "Only SecureML, Sarda, Gazelle, LeNet, AlexNet, and VGG16 Networks supported");
}
void runOnly(NeuralNetwork* net, size_t l, string what, string& network)
{
	size_t total_layers = net->layers.size();
	assert((l >= 0 and l < total_layers) && "Incorrect layer number for runOnly"); 
	network = network + " L" + std::to_string(l) + " " + what;

	if (what.compare("F") == 0)
	{
		if (l == 0)
			net->layers[0]->forward(net->inputData);
		else
			net->layers[l]->forward(*(net->layers[l-1]->getActivation()));
	}
	else if (what.compare("D") == 0)
	{
		if (l != 0)
			net->layers[l]->computeDelta(*(net->layers[l-1]->getDelta()));	
	}
	else if (what.compare("U") == 0)
	{
		if (l == 0)
			net->layers[0]->updateEquations(net->inputData);
		else
			net->layers[l]->updateEquations(*(net->layers[l-1]->getActivation()));
	}
	else
		assert(false && "Only F,D or U allowed in runOnly");
}






/********************* COMMUNICATION AND HELPERS *********************/

void start_m()
{
	// cout << endl;
	start_time();
	start_communication();
}

void end_m(string str)
{
	end_time(str);
	pause_communication();
	aggregateCommunication();
	end_communication(str);
}

void start_time()
{
	if (alreadyMeasuringTime)
	{
		cout << "Nested timing measurements" << endl;
		exit(-1);
	}

	tStart = clock();
	clock_gettime(CLOCK_REALTIME, &requestStart);
	alreadyMeasuringTime = true;
}

void end_time(string str)
{
	if (!alreadyMeasuringTime)
	{
		cout << "start_time() never called" << endl;
		exit(-1);
	}

	clock_gettime(CLOCK_REALTIME, &requestEnd);
	cout << "----------------------------------------------" << endl;
	cout << "Wall Clock time for " << str << ": " << diff(requestStart, requestEnd) << " sec\n";
	cout << "CPU time for " << str << ": " << (double)(clock() - tStart)/CLOCKS_PER_SEC << " sec\n";
	cout << "----------------------------------------------" << endl;	
	alreadyMeasuringTime = false;
}


void start_rounds()
{
	if (alreadyMeasuringRounds)
	{
		cout << "Nested round measurements" << endl;
		exit(-1);
	}

	roundComplexitySend = 0;
	roundComplexityRecv = 0;
	alreadyMeasuringRounds = true;
}

void end_rounds(string str)
{
	if (!alreadyMeasuringTime)
	{
		cout << "start_rounds() never called" << endl;
		exit(-1);
	}

	cout << "----------------------------------------------" << endl;
	cout << "Send Round Complexity of " << str << ": " << roundComplexitySend << endl;
	cout << "Recv Round Complexity of " << str << ": " << roundComplexityRecv << endl;
	cout << "----------------------------------------------" << endl;	
	alreadyMeasuringRounds = false;
}

void aggregateCommunication()
{
	vector<myType> vec(4, 0), temp(4, 0);
	vec[0] = commObject.getSent();
	vec[1] = commObject.getRecv();
	vec[2] = commObject.getRoundsSent();
	vec[3] = commObject.getRoundsRecv();

	if (partyNum == PARTY_B or partyNum == PARTY_C)
		sendVector<myType>(vec, PARTY_A, 4);

	if (partyNum == PARTY_A)
	{
		receiveVector<myType>(temp, PARTY_B, 4);
		for (size_t i = 0; i < 4; ++i)
			vec[i] = temp[i] + vec[i];
		receiveVector<myType>(temp, PARTY_C, 4);
		for (size_t i = 0; i < 4; ++i)
			vec[i] = temp[i] + vec[i];
	}

	if (partyNum == PARTY_A)
	{
		cout << "----------------------------------------------" << endl;
		cout << "Total communication: " << (double)vec[0]/1000000 << "MB (sent) and " << (double)vec[1]/1000000 << "MB (recv)\n";
		cout << "Total calls: " << vec[2] << " (sends) and " << vec[3] << " (recvs)" << endl;
		cout << "----------------------------------------------" << endl;
	}
}


void print_usage (const char * bin) 
{
    cout << "Usage: ./" << bin << " PARTY_NUM IP_ADDR_FILE AES_SEED_INDEP AES_SEED_NEXT AES_SEED_PREV" << endl;
    cout << endl;
    cout << "Required Arguments:\n";
    cout << "PARTY_NUM			Party Identifier (0,1, or 2)\n";
    cout << "IP_ADDR_FILE		\tIP Address file (use makefile for automation)\n";
    cout << "AES_SEED_INDEP		\tAES seed file independent\n";
    cout << "AES_SEED_NEXT		\t \tAES seed file next\n";
    cout << "AES_SEED_PREV		\t \tAES seed file previous\n";
    cout << endl;
    cout << "Report bugs to swagh@princeton.edu" << endl;
    exit(-1);
}

double diff(timespec start, timespec end)
{
    timespec temp;

    if ((end.tv_nsec-start.tv_nsec)<0)
    {
            temp.tv_sec = end.tv_sec-start.tv_sec-1;
            temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
    }
    else 
    {
            temp.tv_sec = end.tv_sec-start.tv_sec;
            temp.tv_nsec = end.tv_nsec-start.tv_nsec;
    }
    return temp.tv_sec + (double)temp.tv_nsec/NANOSECONDS_PER_SEC;
}


void deleteObjects()
{
	//close connection
	for (int i = 0; i < NUM_OF_PARTIES; i++)
	{
		if (i != partyNum)
		{
			delete communicationReceivers[i];
			delete communicationSenders[i];
		}
	}
	delete[] communicationReceivers;
	delete[] communicationSenders;
	delete[] addrs;
}


/************************ AlexNet on ImageNet ************************/
// NUM_LAYERS = 21;
// WITH_NORMALIZATION = false;
// CNNConfig* l0 = new CNNConfig(227,227,3,96,11,4,0,MINI_BATCH_SIZE);
// MaxpoolConfig* l1 = new MaxpoolConfig(55,55,96,3,2,MINI_BATCH_SIZE);
// ReLUConfig* l2 = new ReLUConfig(27*27*96,MINI_BATCH_SIZE);		
// BNConfig * l3 = new BNConfig(27*27*96,MINI_BATCH_SIZE);

// CNNConfig* l4 = new CNNConfig(27,27,96,256,5,1,2,MINI_BATCH_SIZE);
// MaxpoolConfig* l5 = new MaxpoolConfig(27,27,256,3,2,MINI_BATCH_SIZE);
// ReLUConfig* l6 = new ReLUConfig(13*13*256,MINI_BATCH_SIZE);		
// BNConfig * l7 = new BNConfig(13*13*256,MINI_BATCH_SIZE);

// CNNConfig* l8 = new CNNConfig(13,13,256,384,3,1,1,MINI_BATCH_SIZE);
// ReLUConfig* l9 = new ReLUConfig(13*13*384,MINI_BATCH_SIZE);
// CNNConfig* l10 = new CNNConfig(13,13,384,384,3,1,1,MINI_BATCH_SIZE);
// ReLUConfig* l11 = new ReLUConfig(13*13*384,MINI_BATCH_SIZE);
// CNNConfig* l12 = new CNNConfig(13,13,384,256,3,1,1,MINI_BATCH_SIZE);
// MaxpoolConfig* l13 = new MaxpoolConfig(13,13,256,3,2,MINI_BATCH_SIZE);
// ReLUConfig* l14 = new ReLUConfig(6*6*256,MINI_BATCH_SIZE);

// FCConfig* l15 = new FCConfig(6*6*256,MINI_BATCH_SIZE,4096);
// ReLUConfig* l16 = new ReLUConfig(4096,MINI_BATCH_SIZE);
// FCConfig* l17 = new FCConfig(4096,MINI_BATCH_SIZE,4096);
// ReLUConfig* l18 = new ReLUConfig(4096,MINI_BATCH_SIZE);
// FCConfig* l19 = new FCConfig(4096,MINI_BATCH_SIZE,1000);
// ReLUConfig* l20 = new ReLUConfig(1000,MINI_BATCH_SIZE);
// config->addLayer(l0);
// config->addLayer(l1);
// config->addLayer(l2);
// config->addLayer(l3);
// config->addLayer(l4);
// config->addLayer(l5);
// config->addLayer(l6);
// config->addLayer(l7);
// config->addLayer(l8);
// config->addLayer(l9);
// config->addLayer(l10);
// config->addLayer(l11);
// config->addLayer(l12);
// config->addLayer(l13);
// config->addLayer(l14);
// config->addLayer(l15);
// config->addLayer(l16);
// config->addLayer(l17);
// config->addLayer(l18);
// config->addLayer(l19);
// config->addLayer(l20);


/************************ VGG16 on ImageNet ************************/
// NUM_LAYERS = 37;
// WITH_NORMALIZATION = false;
// CNNConfig* l0 = new CNNConfig(224,224,3,64,3,1,1,MINI_BATCH_SIZE);
// ReLUConfig* l1 = new ReLUConfig(224*224*64,MINI_BATCH_SIZE);		
// CNNConfig* l2 = new CNNConfig(224,224,64,64,3,1,1,MINI_BATCH_SIZE);
// MaxpoolConfig* l3 = new MaxpoolConfig(224,224,64,2,2,MINI_BATCH_SIZE);
// ReLUConfig* l4 = new ReLUConfig(112*112*64,MINI_BATCH_SIZE);

// CNNConfig* l5 = new CNNConfig(112,112,64,128,3,1,1,MINI_BATCH_SIZE);
// ReLUConfig* l6 = new ReLUConfig(112*112*128,MINI_BATCH_SIZE);
// CNNConfig* l7 = new CNNConfig(112,112,128,128,3,1,1,MINI_BATCH_SIZE);
// MaxpoolConfig* l8 = new MaxpoolConfig(112,112,128,2,2,MINI_BATCH_SIZE);
// ReLUConfig* l9 = new ReLUConfig(56*56*128,MINI_BATCH_SIZE);

// CNNConfig* l10 = new CNNConfig(56,56,128,256,3,1,1,MINI_BATCH_SIZE);
// ReLUConfig* l11 = new ReLUConfig(56*56*256,MINI_BATCH_SIZE);
// CNNConfig* l12 = new CNNConfig(56,56,256,256,3,1,1,MINI_BATCH_SIZE);
// ReLUConfig* l13 = new ReLUConfig(56*56*256,MINI_BATCH_SIZE);
// CNNConfig* l14 = new CNNConfig(56,56,256,256,3,1,1,MINI_BATCH_SIZE);
// MaxpoolConfig* l15 = new MaxpoolConfig(56,56,256,2,2,MINI_BATCH_SIZE);
// ReLUConfig* l16 = new ReLUConfig(28*28*256,MINI_BATCH_SIZE);

// CNNConfig* l17 = new CNNConfig(28,28,256,512,3,1,1,MINI_BATCH_SIZE);
// ReLUConfig* l18 = new ReLUConfig(28*28*512,MINI_BATCH_SIZE);
// CNNConfig* l19 = new CNNConfig(28,28,512,512,3,1,1,MINI_BATCH_SIZE);
// ReLUConfig* l20 = new ReLUConfig(28*28*512,MINI_BATCH_SIZE);
// CNNConfig* l21 = new CNNConfig(28,28,512,512,3,1,1,MINI_BATCH_SIZE);
// MaxpoolConfig* l22 = new MaxpoolConfig(28,28,512,2,2,MINI_BATCH_SIZE);
// ReLUConfig* l23 = new ReLUConfig(14*14*512,MINI_BATCH_SIZE);

// CNNConfig* l24 = new CNNConfig(14,14,512,512,3,1,1,MINI_BATCH_SIZE);
// ReLUConfig* l25 = new ReLUConfig(14*14*512,MINI_BATCH_SIZE);
// CNNConfig* l26 = new CNNConfig(14,14,512,512,3,1,1,MINI_BATCH_SIZE);
// ReLUConfig* l27 = new ReLUConfig(14*14*512,MINI_BATCH_SIZE);
// CNNConfig* l28 = new CNNConfig(14,14,512,512,3,1,1,MINI_BATCH_SIZE);
// MaxpoolConfig* l29 = new MaxpoolConfig(14,14,512,2,2,MINI_BATCH_SIZE);
// ReLUConfig* l30 = new ReLUConfig(7*7*512,MINI_BATCH_SIZE);

// FCConfig* l31 = new FCConfig(7*7*512,MINI_BATCH_SIZE,4096);
// ReLUConfig* l32 = new ReLUConfig(4096,MINI_BATCH_SIZE);
// FCConfig* l33 = new FCConfig(4096, MINI_BATCH_SIZE, 4096);
// ReLUConfig* l34 = new ReLUConfig(4096, MINI_BATCH_SIZE);
// FCConfig* l35 = new FCConfig(4096, MINI_BATCH_SIZE, 1000);
// ReLUConfig* l36 = new ReLUConfig(1000, MINI_BATCH_SIZE);
// config->addLayer(l0);
// config->addLayer(l1);
// config->addLayer(l2);
// config->addLayer(l3);
// config->addLayer(l4);
// config->addLayer(l5);
// config->addLayer(l6);
// config->addLayer(l7);
// config->addLayer(l8);
// config->addLayer(l9);
// config->addLayer(l10);
// config->addLayer(l11);
// config->addLayer(l12);
// config->addLayer(l13);
// config->addLayer(l14);
// config->addLayer(l15);
// config->addLayer(l16);
// config->addLayer(l17);
// config->addLayer(l18);
// config->addLayer(l19);
// config->addLayer(l20);
// config->addLayer(l21);
// config->addLayer(l22);
// config->addLayer(l23);
// config->addLayer(l24);
// config->addLayer(l25);
// config->addLayer(l26);
// config->addLayer(l27);
// config->addLayer(l28);
// config->addLayer(l29);
// config->addLayer(l30);
// config->addLayer(l31);
// config->addLayer(l32);
// config->addLayer(l33);
// config->addLayer(l34);
// config->addLayer(l35);
// config->addLayer(l36);
