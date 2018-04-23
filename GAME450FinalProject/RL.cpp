#include <iostream>
#include <time.h>
#include <cmath>
#include <fstream>
using namespace std;

struct Turn {
	int myAction;
	int oppAction;
	int damageDealt;
	int damageTaken;
	int oppHealthLow;
	int oppHealthCritical;
	int myGlobe;
	int myStoneskin;
	int myStrength;
	int oppGlobe;
	int oppStoneskin;
	int oppStrength;
	int roundEnded;
};

struct GameState {
	int health = 100;
	bool healingPotion = true;
	bool strengthPotion = true;
};

const int IDLE = 0;
const int MISSILE = 1;
const int FIREBALL = 2;
const int DAGGER = 3;
const int STAFF = 4;
const int GLOBE = 5;
const int STONESKIN = 6;
const int HEAL = 7;
const int STRENGTH = 8;

const int numAction = 9; // 9 actions 
const int numFeature = 11; // 11 features: alive, self globe, self stoneskin, self strength, self low health, self critical,
// enemy globe, enemy stoneskin, enemy strength, enemy low health and enemy critical

float alpha = 0.85f;
float gamma = 0.55f;
float epsilon = 0.90f;

float weights[numAction][numFeature]; // the weight matrix used for functon approximation
int features[numFeature]; // boolean features with values 1 or 0

int selectAction(const GameState &playerState, const GameState &opponentState);
void updateWeights(int action, float reward, const int oldFeatures[numFeature]);
void printWeights();

int main() {
	srand(time(NULL));

	// Check to see if files exist
	ifstream fin;
	fin.open("memory.txt");
	if (fin.is_open()) {
		// load weights in from file
		for (int i = 0; i < numAction; i++) {
			for (int j = 0; j < numFeature; j++) {
				fin >> weights[i][j];
			}
		}
	} else {
		// random initialization of weights
		for (int i = 0; i < numAction; i++) {
			for (int j = 0; j < numFeature; j++) {
				weights[i][j] = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
			}
		}
	}
	fin.close();

	features[0] = 1;
	for (int i = 1; i < numFeature; i++) {
		features[i] = 0;
	}

	GameState myState;
	GameState oppState;

	int numRounds;
	cin >> numRounds;

	for (int i = 0; i < numRounds; i++) {
		while (true) { // Game Loop
			Turn currentTurn;

			// Print the weights of the action
			printWeights();

			// Select action
			currentTurn.myAction = selectAction(myState, oppState);
			cout << currentTurn.myAction << endl;

			// Read in the rest of the turn's data
			cin >> currentTurn.oppAction >> currentTurn.damageDealt >> currentTurn.damageTaken >> currentTurn.oppHealthLow >>
				currentTurn.oppHealthCritical >> currentTurn.myGlobe >> currentTurn.myStoneskin >> currentTurn.myStrength >>
				currentTurn.oppGlobe >> currentTurn.oppStoneskin >> currentTurn.oppStrength >> currentTurn.roundEnded;

			if (currentTurn.roundEnded != 0) {
				break;
			}

			// Update State
			myState.health -= currentTurn.damageTaken;
			if (myState.health < 0) {
				myState.health = 0;
			} else if (myState.health > 100) {
				myState.health = 100;
			}
			switch (currentTurn.myAction) {
			case HEAL:
				myState.healingPotion = false;
				break;
			case STRENGTH:
				myState.strengthPotion = false;
				break;
			}

			oppState.health -= currentTurn.damageDealt;
			if (oppState.health < 0) {
				oppState.health = 0;
			} else if (oppState.health > 100) {
				oppState.health = 100;
			}
			switch (currentTurn.oppAction) {
			case HEAL:
				oppState.healingPotion = false;
				break;
			case STRENGTH:
				oppState.strengthPotion = false;
				break;
			}

			// Store old features
			int oldFeatures[numFeature];
			for (int i = 0; i < numFeature; i++) {
				oldFeatures[i] = features[i];
			}


			// Update features
			features[1] = currentTurn.myGlobe;
			features[2] = currentTurn.myStoneskin;
			features[3] = currentTurn.myStrength;
			features[4] = myState.health < 50 ? 1 : 0;
			features[5] = myState.health < 10 ? 1 : 0;
			features[6] = currentTurn.oppGlobe;
			features[7] = currentTurn.oppStoneskin;
			features[8] = currentTurn.oppStrength;
			features[9] = currentTurn.oppHealthLow;
			features[10] = currentTurn.oppHealthCritical;

			// Update Weights
			float reward = static_cast<float>(currentTurn.damageDealt - currentTurn.damageTaken);
			updateWeights(currentTurn.myAction, reward, oldFeatures);

		}
		epsilon -= 0.05f;
	}

	// Save the new weights for the player
	ofstream fout;
	fout.open("memory.txt");
	if (fout.is_open()) {
		for (int i = 0; i < numAction; i++) {
			for (int j = 0; j < numFeature; j++) {
				fout << weights[i][j] << " ";
			}
		}
	}

	cerr << endl;
	cout.flush();
	return 0;

}

// selects an action based on reinforcement learning
int selectAction(const GameState &playerState, const GameState &opponentState) {
	int action = IDLE;
	float randomValue = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
	if (randomValue < epsilon) {
		action = rand() % numAction;
		while ((action == STRENGTH && !playerState.strengthPotion) || (action == HEAL && !playerState.healingPotion)) {
			action = rand() % numAction;
		}
	} else {
		float currentWeight = -100000.f;
		for (int i = 0; i < numAction; i++) {
			if ((i == STRENGTH && !playerState.strengthPotion) || (i == HEAL && !playerState.healingPotion)) {
				continue;
			}
			float resultWeight = 0.f;
			for (int j = 0; j < numFeature; j++) {
				if (features[j] == 1) {
					resultWeight += weights[i][j];
				}
			}
			if (resultWeight > currentWeight) {
				currentWeight = resultWeight;
				action = i;
			}
		}
	}
	return action;
}

// updates the weight based on the action
void updateWeights(int action, float reward, const int oldFeatures[numFeature]) {
	float oldWeight = 0.f;
	for (int i = 0; i < numFeature; i++) {
		oldWeight += weights[action][i] * oldFeatures[i];
	}
	float maxWeight = -10000000000.f;
	for (int i = 0; i < numAction; i++) {
		float sumWeights = 0.f;
		for (int j = 0; j < numFeature; j++) {
			sumWeights += weights[i][j] * features[j];
		}
		if (sumWeights > maxWeight) {
			maxWeight = sumWeights;
		}
	}
	for (int i = 0; i < numFeature; i++) {
		if (oldFeatures[i] == 1) {
			weights[action][i] = weights[action][i] + alpha * (reward + gamma * maxWeight - oldWeight);
		}
	}
}


// prints the weight matrix and the features in the current state
void printWeights() {
	cerr << "Weight matrix: " << endl;
	for (int i = 0; i < numAction; i++) {
		for (int j = 0; j < numFeature; j++) {
			cerr << weights[i][j] << " ";
		}
		cerr << endl;
	}

	cerr << "Features in the state: " << endl;
	for (int i = 0; i < numFeature; i++) {
		cerr << features[i] << " ";
	}
	cerr << endl;
}