//Purpose: parse the departing results and arriving results from the verilog design 
//Author: Jiayi Sheng
//Organization: CAAD lab @ Boston University
//Start date: Jan 13th 2016
//Input: dump.txt

#include <fstream>
#include <iostream>
#include<string>
#include<stdlib.h>
#include<time.h>
#include<queue>

using namespace std;

//departure packet is format [src.x] [src.y] [src.z] [id] [time] [packet type]
//arrival packet is format   [src.x] [src.y] [src.z] [dst.x] [dst.y] [dst.z] [id] [time] [packet type]



#define LINEMAX 100

#define PARTICLE_PER_BOX 192
#define MODE 2 // 1 is multicast mode, 2 is reduction mode, 3 is the singlecast multicastmode, 4 is the singlecast reduction mode
#define MAX_NUM_CHILDREN 6

#define LOCAL 0
#define YNEG 1
#define YPOS 2
#define XPOS 3
#define XNEG 4
#define ZPOS 5
#define ZNEG 6
#define X 4
#define Y 4
#define Z 4

int export_num;



double r2c = 1;// the ratio between the cutoff radius and box size
int particle_per_box = PARTICLE_PER_BOX;
int particle_per_cell = PARTICLE_PER_BOX;
float r = 12;
float xsize = 108;
float ysize = 108;
float zsize = 80;

int mode = MODE;

struct packet_timing{
	int depart_time;
	int arrival_time[X*Y*Z];
	int valid;
};

struct singlecast_packet_timing{
	int depart_counter;
	int arrival_counter;
	int valid;
};

struct reduction_packet_timing{
	int depart_time[X*Y*Z];
	int arrival_time;
	int valid;
};

int avg_reduction_departure_time[PARTICLE_PER_BOX];
int reduction_depart_counter[PARTICLE_PER_BOX];
int avg_reduction_arrival_time[PARTICLE_PER_BOX];
int latest_reduction_arrival_time;
int avg_reduction_depart_counter;
struct packet_timing** multicast_timing;
struct reduction_packet_timing** reduction_timing;


void read_dump_singlecast_multicast(char* filename){
	ifstream input_file;
	int line_counter = 0;
	input_file.open(filename);
	char *tokens;// the number is going to be read from the dump.txt
	int src_x;
	int src_y;
	int src_z;
	int dst_x;
	int dst_y;
	int dst_z;
	int id;
	int depart_time;
	int arrival_time;
	char line[LINEMAX];
	while (!input_file.eof()){
		input_file.getline(line, LINEMAX);
		line_counter++;
		if (line[0] == 'D'){
			input_file.getline(line, LINEMAX);
			line_counter++;
			tokens = strtok(line, " ");
			src_x = atoi(tokens);
			tokens = strtok(NULL, " ");
			src_y = atoi(tokens);
			tokens = strtok(NULL, " ");
			src_z = atoi(tokens);
			tokens = strtok(NULL, " ");
			id = atoi(tokens);
			tokens = strtok(NULL, " ");
			depart_time = atoi(tokens);
			if (multicast_timing[src_x*Y*Z + src_y*Z + src_z][id].valid == false){
				multicast_timing[src_x*Y*Z + src_y*Z + src_z][id].valid = true;
				multicast_timing[src_x*Y*Z + src_y*Z + src_z][id].depart_time = depart_time;
				for (int i = 0; i < X*Y*Z; i++){
					multicast_timing[src_x*Y*Z + src_y*Z + src_z][id].arrival_time[i] = -1;
				}
			}
			

		}
		else if (line[0] == 'A'){
			input_file.getline(line, LINEMAX);
			line_counter++;
			tokens = strtok(line, " ");
			src_x = atoi(tokens);
			tokens = strtok(NULL, " ");
			src_y = atoi(tokens);
			tokens = strtok(NULL, " ");
			src_z = atoi(tokens);
			tokens = strtok(NULL, " ");
			dst_x = atoi(tokens);
			tokens = strtok(NULL, " ");
			dst_y = atoi(tokens);
			tokens = strtok(NULL, " ");
			dst_z = atoi(tokens);
			tokens = strtok(NULL, " ");
			id = atoi(tokens);

			tokens = strtok(NULL, " ");
			arrival_time = atoi(tokens);

				multicast_timing[src_x*Y*Z + src_y*Z + src_z][id].arrival_time[dst_x*Y*Z + dst_y*Z + dst_z] = arrival_time;

			
		}


	}
	input_file.close();
}

void read_dump_singlecast_reduction(char* filename){
	ifstream input_file;
	int line_counter = 0;
	input_file.open(filename);
	if (input_file.fail()){
		cout << "open file failed" << filename << endl;
		return;
	}
	char *tokens;// the number is going to be read from the dump.txt
	int src_x;
	int src_y;
	int src_z;
	int dst_x;
	int dst_y;
	int dst_z;
	int id;
	int depart_time;
	int arrival_time;
	char line[LINEMAX];
	for (int i = 0; i < PARTICLE_PER_BOX; i++){
		avg_reduction_departure_time[i] = 0;
		reduction_depart_counter[i] = 0;
		avg_reduction_arrival_time[i] = 0;
	}

	while (!input_file.eof()){
		input_file.getline(line, LINEMAX);
		line_counter++;
		if (line[0] == 'D'){
			input_file.getline(line, LINEMAX);
			line_counter++;
			tokens = strtok(line, " ");
			src_x = atoi(tokens);
			tokens = strtok(NULL, " ");
			src_y = atoi(tokens);
			tokens = strtok(NULL, " ");
			src_z = atoi(tokens);
			tokens = strtok(NULL, " ");
			id = atoi(tokens);
			tokens = strtok(NULL, " ");
			depart_time = atoi(tokens);
			avg_reduction_departure_time[id] += depart_time;
			reduction_depart_counter[id]++;


		}
		else if (line[0] == 'A'){
			input_file.getline(line, LINEMAX);
			line_counter++;
			tokens = strtok(line, " ");
			src_x = atoi(tokens);
			tokens = strtok(NULL, " ");
			src_y = atoi(tokens);
			tokens = strtok(NULL, " ");
			src_z = atoi(tokens);
			tokens = strtok(NULL, " ");
			dst_x = atoi(tokens);
			tokens = strtok(NULL, " ");
			dst_y = atoi(tokens);
			tokens = strtok(NULL, " ");
			dst_z = atoi(tokens);
			tokens = strtok(NULL, " ");
			id = atoi(tokens);

			tokens = strtok(NULL, " ");
			arrival_time = atoi(tokens);
			if (reduction_timing[dst_x*Y*Z + dst_y*Z + dst_z][id].valid == false){
				reduction_timing[dst_x*Y*Z + dst_y*Z + dst_z][id].arrival_time = arrival_time;
				reduction_timing[dst_x*Y*Z + dst_y*Z + dst_z][id].valid = true;
			}
			else{
				if (reduction_timing[dst_x*Y*Z + dst_y*Z + dst_z][id].arrival_time < arrival_time){
					reduction_timing[dst_x*Y*Z + dst_y*Z + dst_z][id].arrival_time = arrival_time;
				}
			}


		}


	}

	for (int i = 0; i < PARTICLE_PER_BOX; i++){
		avg_reduction_departure_time[i] = avg_reduction_departure_time[i] / reduction_depart_counter[i];
	}



	input_file.close();
}

void read_dump_reduction(char* filename){
	ifstream input_file;
	int line_counter = 0;
	input_file.open(filename);
	if (input_file.fail()){
		cout << "open file failed" << filename << endl;
		return;
	}
	char *tokens;// the number is going to be read from the dump.txt
	int src_x;
	int src_y;
	int src_z;
	int dst_x;
	int dst_y;
	int dst_z;
	int id;
	int depart_time;
	int arrival_time;
	char line[LINEMAX];
	for (int i = 0; i < PARTICLE_PER_BOX; i++){
		avg_reduction_departure_time[i] =0;
		reduction_depart_counter[i]=0;
		avg_reduction_arrival_time[i] = 0;
	}

	while (!input_file.eof()){
		input_file.getline(line, LINEMAX);
		line_counter++;
		if (line[0] == 'D'){
			input_file.getline(line, LINEMAX);
			line_counter++;
			tokens = strtok(line, " ");
			src_x = atoi(tokens);
			tokens = strtok(NULL, " ");
			src_y = atoi(tokens);
			tokens = strtok(NULL, " ");
			src_z = atoi(tokens);
			tokens = strtok(NULL, " ");
			id = atoi(tokens);
			tokens = strtok(NULL, " ");
			depart_time = atoi(tokens);
			avg_reduction_departure_time[id] += depart_time;
			reduction_depart_counter[id]++;
			

		}
		else if (line[0] == 'A'){
			input_file.getline(line, LINEMAX);
			line_counter++;
			tokens = strtok(line, " ");
			src_x = atoi(tokens);
			tokens = strtok(NULL, " ");
			src_y = atoi(tokens);
			tokens = strtok(NULL, " ");
			src_z = atoi(tokens);
			tokens = strtok(NULL, " ");
			dst_x = atoi(tokens);
			tokens = strtok(NULL, " ");
			dst_y = atoi(tokens);
			tokens = strtok(NULL, " ");
			dst_z = atoi(tokens);
			tokens = strtok(NULL, " ");
			id = atoi(tokens);

			tokens = strtok(NULL, " ");
			arrival_time = atoi(tokens);
			if (arrival_time > latest_reduction_arrival_time)
				latest_reduction_arrival_time = arrival_time;
			reduction_timing[dst_x*Y*Z + dst_y*Z + dst_z][id].arrival_time = arrival_time;
			reduction_timing[dst_x*Y*Z + dst_y*Z + dst_z][id].valid = true;


		}


	}

	for (int i = 0; i < PARTICLE_PER_BOX; i++){
		avg_reduction_departure_time[i] = avg_reduction_departure_time[i] / reduction_depart_counter[i];
	}

	avg_reduction_depart_counter = 0;
	int valid_node_count = 0;
	for (int i = 0; i < X*Y*Z; i++){
		if (reduction_depart_counter>0){
			valid_node_count++;
			avg_reduction_depart_counter += reduction_depart_counter[i];
		}
		
	}
	avg_reduction_depart_counter = avg_reduction_depart_counter / valid_node_count;
	

	
	input_file.close();

}




void read_dump_multicast(char* filename){
	ifstream input_file;
	int line_counter = 0;
	input_file.open(filename);
	char *tokens;// the number is going to be read from the dump.txt
	int src_x;
	int src_y;
	int src_z;
	int dst_x;
	int dst_y;
	int dst_z;
	int id;
	int depart_time;
	int arrival_time;

	char line[LINEMAX];
	while (!input_file.eof()){
		input_file.getline(line, LINEMAX);
		line_counter++;
		if (line[0] == 'D'){
			input_file.getline(line, LINEMAX);
			line_counter++;
			tokens = strtok(line, " ");
			src_x = atoi(tokens);
			tokens = strtok(NULL, " ");
			src_y = atoi(tokens);
			tokens = strtok(NULL, " ");
			src_z = atoi(tokens);
			tokens = strtok(NULL, " ");
			id = atoi(tokens);
			tokens = strtok(NULL, " ");
			depart_time = atoi(tokens);
			multicast_timing[src_x*Y*Z + src_y*Z + src_z][id].depart_time = depart_time;
			multicast_timing[src_x*Y*Z + src_y*Z + src_z][id].valid = true;
			for (int i = 0; i < X*Y*Z; i++){
				multicast_timing[src_x*Y*Z + src_y*Z + src_z][id].arrival_time[i] = -1;
			}

		}
		else if (line[0] == 'A'){
			input_file.getline(line, LINEMAX);
			line_counter++;
			tokens = strtok(line, " ");
			src_x = atoi(tokens);
			tokens = strtok(NULL, " ");
			src_y = atoi(tokens);
			tokens = strtok(NULL, " ");
			src_z = atoi(tokens);
			tokens = strtok(NULL, " ");
			dst_x = atoi(tokens);
			tokens = strtok(NULL, " ");
			dst_y = atoi(tokens);
			tokens = strtok(NULL, " ");
			dst_z = atoi(tokens);
			tokens = strtok(NULL, " ");
			id = atoi(tokens);

			tokens = strtok(NULL, " ");
			arrival_time = atoi(tokens);
			multicast_timing[src_x*Y*Z + src_y*Z + src_z][id].arrival_time[dst_x*Y*Z + dst_y*Z + dst_z] = arrival_time;



		}


	}
	input_file.close();
}

void verify_reduction(){
	for (int i_x = 0; i_x < X; i_x++){
		for (int i_y = 0; i_y < Y; i_y++){
			for (int i_z = 0; i_z < Z; i_z++){
				int i = i_x*X*Y + i_y*Y + i_z;
				for (int j = 0; j < PARTICLE_PER_BOX; j++){
					if (reduction_timing[i][j].valid == false){
						cout << "error at" << i_x << " " << i_y << " " << i_z << " id is " << j << endl;
					}
				}
			}
		}

	}

	for (int i = 0; i < PARTICLE_PER_BOX; i++){
		for (int j = 0; j < X*Y*Z; j++){
			avg_reduction_arrival_time[i] += reduction_timing[j][i].arrival_time;
		}
		avg_reduction_arrival_time[i] = avg_reduction_arrival_time[i] / (X*Y*Z); 
	}
	int latency = 0;
	int valid_counter = 0;
	for (int i = 0; i < PARTICLE_PER_BOX; i++){
		if (avg_reduction_arrival_time[i] != 0 && avg_reduction_departure_time[i] != 0){
			latency += avg_reduction_arrival_time[i] - avg_reduction_departure_time[i];
			valid_counter++;
		}
	}
	latency = latency / valid_counter;
	cout << "avg latency is " << latency *6.4 << endl;
	cout << "actual latency has not times 6.4 is" << latest_reduction_arrival_time;
	
}

void verify_multicast(){
	int bitmap[X*Y*Z];
	int ref_counter = 0;
	int counter;
	bool find_ref=false;
	for (int i = 0; i < X*Y*Z; i++){
		bitmap[i] = 0;
	}
	for (int i = 0; i < X*Y*Z; i++){
		for (int j = 0; j < PARTICLE_PER_BOX; j++){
			if (multicast_timing[i][j].valid){
				find_ref=true;
				for (int k = 0; k < X*Y*Z; k++){
					if (multicast_timing[i][j].arrival_time[k] != -1){
						ref_counter++;
					}
				}
				break;
			}
			
		}
		if(find_ref)
			break;
	}
	
	for (int i = 0; i < X*Y*Z; i++){
		for (int j = 0; j < PARTICLE_PER_BOX; j++){
			if (multicast_timing[i][j].valid){
				counter = 0;
				for (int k = 0; k < X*Y*Z; k++){
					if (multicast_timing[i][j].arrival_time[k] != -1){
						counter++;
					}
				}
				if (counter != ref_counter) {
					cout << "error at" << i << " " << j << endl;
					cout << "counter=" << counter << " ref_counter=" << ref_counter << endl;
				}
			}
		}
	}
	cout << "all correct!" << endl;
	int last_arrival_time = 0;
	int depart_time = 0;
	int aggregate_latency = 0;
	int latency = 0;
	float avg_latency = 0;
	int total_valid_counter = 0;
	for (int i = 0; i<X*Y*Z; i++){
		for (int j = 0; j<PARTICLE_PER_BOX; j++){
			if (multicast_timing[i][j].valid){
				total_valid_counter++;
				depart_time = multicast_timing[i][j].depart_time;
				last_arrival_time = 0;
				for (int k = 0; k<X*Y*Z; k++){
					if (multicast_timing[i][j].arrival_time[k]>last_arrival_time){
						last_arrival_time = multicast_timing[i][j].arrival_time[k];
					}
				}
				latency = last_arrival_time - depart_time;
				aggregate_latency += latency;
			}
		}
	}
	avg_latency = (float)aggregate_latency / (float)(total_valid_counter);
	cout << "avg latency is" << avg_latency*6.4 << endl;
	
}

/*void verify_singlecast(){
	for (int i = 0; i < X*Y*Z; i++){
		for (int j = 0; j < PARTICLE_PER_BOX; j++){
			if (singlecast_timing[i][j].arrival_counter < singlecast_timing[i][j].depart_counter){
				cout << "error at" << i << " " << j << endl;
				cout << "depart counter=" << singlecast_timing[i][j].depart_counter << " arrival counter=" << singlecast_timing[i][j].arrival_counter << endl;
			}
		}
	}
}*/

int main(){

	char filename[200] = "C:/Users/Jiayi/Documents/GitHub/MD_reduction/software/dump.txt";
	if (mode == 1){
		if (!(multicast_timing = (packet_timing**)malloc(X*Y*Z*sizeof(packet_timing*)))){
			cout << "No mem for multicast_timing" << endl;
			exit(-1);
		}
		for (int i = 0; i < X*Y*Z; i++){
			if (!(multicast_timing[i] = (packet_timing*)malloc(PARTICLE_PER_BOX*sizeof(packet_timing)))){
				cout << "No mem for multicast_timing" << i << endl;
				exit(-1);
			}
		}
		//init multicast_timing
		for (int i = 0; i < X*Y*Z; i++){
			for (int j = 0; j < PARTICLE_PER_BOX; j++){
				//multicast_timing[i][j].arrival_time = 0;
				multicast_timing[i][j].depart_time = 0;
				multicast_timing[i][j].valid = false;
			}
		}
		read_dump_multicast(filename);
		verify_multicast();

	}
	else if (mode == 2){
		if (!(reduction_timing = (reduction_packet_timing**)malloc(X*Y*Z*sizeof(reduction_packet_timing*)))){
			cout << "No mem for reduction_timing" << endl;
			exit(-1);
		}
		for (int i = 0; i < X*Y*Z; i++){
			if (!(reduction_timing[i] = (reduction_packet_timing*)malloc(PARTICLE_PER_BOX*sizeof(reduction_packet_timing)))){
				cout << "No mem for reduction_timing" << i << endl;
				exit(-1);
			}
		}
		//init multicast_timing
		for (int i = 0; i < X*Y*Z; i++){
			for (int j = 0; j < PARTICLE_PER_BOX; j++){
				//multicast_timing[i][j].arrival_time = 0;
				reduction_timing[i][j].arrival_time = 0;
				reduction_timing[i][j].valid = false;
			}
		}
		read_dump_reduction(filename);
		verify_reduction();
	}
	else if (mode == 3){
		if (!(multicast_timing = (packet_timing**)malloc(X*Y*Z*sizeof(packet_timing*)))){
			cout << "No mem for singlecast_timing" << endl;
			exit(-1);
		}
		for (int i = 0; i < X*Y*Z; i++){
			if (!(multicast_timing[i] = (packet_timing*)malloc(PARTICLE_PER_BOX*sizeof(packet_timing)))){
				cout << "No mem for singlecast_timing" << i << endl;
				exit(-1);
			}
		}
		//init multicast_timing
		for (int i = 0; i < X*Y*Z; i++){
			for (int j = 0; j < PARTICLE_PER_BOX; j++){
				multicast_timing[i][j].depart_time = 0;
				multicast_timing[i][j].valid = false;
			}
		}
		read_dump_singlecast_multicast(filename);
		verify_multicast();

	}
	else if (mode == 4){
		if (!(reduction_timing = (reduction_packet_timing**)malloc(X*Y*Z*sizeof(reduction_packet_timing*)))){
			cout << "No mem for reduction_timing" << endl;
			exit(-1);
		}
		for (int i = 0; i < X*Y*Z; i++){
			if (!(reduction_timing[i] = (reduction_packet_timing*)malloc(PARTICLE_PER_BOX*sizeof(reduction_packet_timing)))){
				cout << "No mem for reduction_timing" << i << endl;
				exit(-1);
			}
		}
		//init multicast_timing
		for (int i = 0; i < X*Y*Z; i++){
			for (int j = 0; j < PARTICLE_PER_BOX; j++){
				//multicast_timing[i][j].arrival_time = 0;
				reduction_timing[i][j].arrival_time = 0;
				reduction_timing[i][j].valid = false;
			}
		}
		read_dump_singlecast_reduction(filename);
		verify_reduction();

	}

	return 0;
}