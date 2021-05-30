#include "stdafx.h"
#include "common.h"
#include <windows.h>

using namespace cv;
using namespace std;

// DATA TYPES

typedef enum {
	WHOLE,
	HALF,
	QUARTER,
	EIGHTH,
	SIXTEENTH,
	INVALID
} NoteType;

typedef enum {
	P_HALF,
	P_QUARTER,
	P_EIGHTH,
	P_INVALID
} PauseType;

typedef enum {
	LA8,
	SOL8,
	FA8,
	MI8,
	RE8,
	DO8,
	SI,
	LA,
	SOL,
	FA,
	MI,
	RE,
	DO,
	UNDEFINED
} NoteValue;

typedef enum {
	UP,
	DOWN,
	RIGHT,
	LEFT,
	N_A //not applicable -> for whole and invalid notes
} Position;

//


// IMAGE CONVERSION FUNCTIONS

Mat_<uchar> convertToGrayscale(Mat_<Vec3b> img) {
	Mat_<uchar> result(img.rows, img.cols);

	for (int i = 0; i < img.rows; i++) {
		for (int j = 0; j < img.cols; j++) {
			result(i, j) = (img(i, j)[0] + img(i, j)[1] + img(i, j)[2]) / 3;
		}
	}

	return result;
}

Mat_<uchar> convertGrayscaleToBinary(Mat_<uchar> img) {
	Mat_<uchar> result(img.rows, img.cols);
	int threshold = 100;
	int execute = 1;
	if (threshold >= 0 || threshold <= 255) {
		for (int i = 0; i < img.rows; i++) {
			for (int j = 0; j < img.cols; j++) {
				img(i, j) < threshold ? result(i, j) = 0 : result(i, j) = 255;
			}
		}
	}
	return result;
}

//

// HELPER FUNCTIONS

bool isInside(Mat img, int i, int j) {
	if ((i >= 0 && i < img.rows) && (j >= 0 && j < img.cols)) {
		return true;
	}
	return false;
}

//

// NOTE STRUCTURAL ELEMENT CONSTANTS

#define NOTE 0
#define PAUSE 1

#define NOTE_HEIGHT_DOWN 40
#define NOTE_HEIGHT_UP 5
#define NOTE_WIDTH 24

#define HALF_PAUSE_HEIGHT 5
#define HALF_PAUSE_WIDTH 18
#define QUARTER_PAUSE_HEIGHT 30
#define QUARTER_PAUSE_WIDTH 9
#define EIGHTH_PAUSE_HEIGHT 17
#define EIGHTH_PAUSE_WIDTH 8

#define RIGHT_LINE_COLUMN_LOCATION(y) y + 11
#define LEFT_LINE_ROW_LOCATION(x) x + 8
#define LINE_LENGTH 32

const pair<NoteType, Position> INVALID_NOTE = { INVALID, N_A };

//

// NOTE IDENTIFICATION FUNCTIONS

bool isStructuralElementInside(Mat_<uchar> img, int x, int y, int TYPE) {
	switch (TYPE) {
	case NOTE:
		if (isInside(img, x + NOTE_HEIGHT_DOWN, y + NOTE_WIDTH) && isInside(img, x - NOTE_HEIGHT_UP, y + NOTE_WIDTH)) {
			return true;
		}
		return false;
	case PAUSE:
		if (isInside(img, x + QUARTER_PAUSE_HEIGHT, y + HALF_PAUSE_WIDTH)) {
			return true;
		}
		return false;
	default:
		return false;
	}
}

Position getLinePosition(Mat_<uchar> img, int x, int y) {
	//search for right line:
	bool hasRightLine = true;
	int j = RIGHT_LINE_COLUMN_LOCATION(y);
	for (int i = x; i <= x + LINE_LENGTH; i++) {
		if (img(i, j) != 0) {
			hasRightLine = false;
			break;
		}
	}
	if (hasRightLine) {
		return RIGHT;
	}

	//search for left line:
	bool hasLeftLine = true;
	j = y;
	int xStart = LEFT_LINE_ROW_LOCATION(x);
	for (int i = xStart; i <= xStart + LINE_LENGTH; i++) {
		if (img(i, j) != 0) {
			hasLeftLine = false;
			break;
		}
	}
	if (hasLeftLine) {
		return LEFT;
	}

	return N_A;
}

bool isWholeNote(Mat_<uchar> img, int x, int y) {

	bool hasVerticalLines = true;
	for (int i = x + 1; i <= x + 9; i++) {
		if (img(i, y + 3) != 0 || img(i, y + 13) != 0) {
			hasVerticalLines = false;
		}
	}


	if (hasVerticalLines) {
		for (int j = y + 4; j <= y + 7; j++) {
			if (img(x + 9, j) != 0) {
				return false;
			}
		}
		for (int j = y + 9; j <= y + 12; j++) {
			if (img(x + 1, j) != 0) {
				return false;
			}
		}
		return true;
	}
	return false;
}

bool hasEllipse(Mat_<uchar> img, int x, int y, Position pos) {
	int i;
	bool hasLineUp = true;
	bool hasLineDown = true;
	switch (pos) {
	case DOWN:
		i = x + 32;
		for (int j = y + 4; j <= y + 10; j++) {
			if (img(i, j) != 0) {
				hasLineUp = false;
				break;
			}
		}
		if (hasLineUp) {
			i = x + 40;
			for (int j = y + 1; j <= y + 7; j++) {
				if (img(i, j) != 0) {
					hasLineDown = false;
					break;
				}
			}
			return hasLineDown;
		}
		return false;
	case UP:
		i = x;
		for (int j = y + 5; j <= y + 10; j++) {
			if (img(i, j) != 0) {
				hasLineUp = false;
				break;
			}
		}
		if (hasLineUp) {
			i = x + 9;
			for (int j = y + 1; j <= y + 6; j++) {
				if (img(i, j) != 0) {
					hasLineDown = false;
					break;
				}
			}
			return hasLineDown;
		}
		return false;
		break;
	default:
		return false;
	}
}

bool isEllipseFilled(Mat_<uchar> img, int x, int y, Position pos) {
	switch (pos) {
	case DOWN:
		if (img(x + 38, y + 3) == 0) {
			return true;
		}
		return false;
	case UP:
		if (img(x + 6, y + 3) == 0) {
			return true;
		}
		return false;
	default:
		return false;
	}
}

bool hasTail(Mat_<uchar> img, int x, int y, Position pos) {
	int j;
	switch (pos) {
	case DOWN:
		j = y + 20;
		for (int i = x + 20; i <= x + 32; i++) {
			if (img(i, j) != 0) {
				return false;
			}
		}
		return true;
	case UP:
		j = y + 11;
		for (int i = x + 13; i <= x + 21; i++) {
			if (img(i, j) != 0) {
				return false;
			}
		}
		return true;
	default:
		return false;
	}
}

pair<NoteType, Position> getNoteType(Mat_<uchar> img, int x, int y) {
	if (!isStructuralElementInside(img, x, y, NOTE)) {
		return INVALID_NOTE;
	}

	Position linePosition = getLinePosition(img, x, y);
	if (linePosition == N_A) { //may be a whole note
		if (isWholeNote(img, x, y)) {
			return { WHOLE, N_A };
		}
	}

	Position ellipseLocation;

	switch (linePosition) {
	case RIGHT:
		ellipseLocation = DOWN;
		break;
	case LEFT:
		ellipseLocation = UP;
		break;
	default:
		ellipseLocation = N_A;
	}

	if (ellipseLocation != N_A) {
		if (hasEllipse(img, x, y, ellipseLocation)) {
			if (isEllipseFilled(img, x, y, ellipseLocation)) {
				if (hasTail(img, x, y, ellipseLocation)) {
					return { EIGHTH, ellipseLocation };
				}
				return { QUARTER, ellipseLocation };
			}
			else {
				return { HALF, ellipseLocation };
			}
		}
	}
	return INVALID_NOTE;
}

bool isHalfPause(Mat_<uchar> img, int x, int y) {
	bool parallelLines = true;

	for (int i = x; i <= x + 4; i++) {
		if (img(i, y + 4) != 0 || img(i, y + 14) != 0) {
			parallelLines = false;
			break;
		}
	}
	bool line = true;
	int i = x + 5;
	for (int j = y; j <= y + 18; j++) {
		if (img(i, j) != 0) {
			line = false;
			break;
		}
		if (line) {
			for (int i = x; i <= x + 4; i++) {
				for (int j = y + 4; j <= y + 14; j++) {
					if (img(i, j) != 0) {
						return false;
					}
				}
			}
			return true;
		}
	}
	return false;
}

bool isQuarterPause(Mat_<uchar> img, int x, int y) {
	bool line1 = true;
	int j;
	j = y;
	for (int i = x + 21; i <= x + 26; i++) {
		if (img(i, j) != 0) {
			line1 = false;
			break;
		}
	}
	if (line1) {
		bool line2 = true;
		int j;
		j = y + 3;
		for (int i = x + 2; i <= x + 17; i++) {
			if (img(i, j) != 0) {
				line2 = false;
				break;
			}
		}
		if (line2) {
			bool line3 = true;
			int j;
			j = y + 5;
			for (int i = x + 4; i <= x + 24; i++) {
				if (img(i, j) != 0) {
					line3 = false;
					break;
				}
			}
			if (line3) {
				return true;
			}
		}
	}
	return false;
}

bool isEighthPause(Mat_<uchar> img, int x, int y) {
	bool line1 = true;
	int i, j;
	i = x + 3;
	for (j = y; j <= y + 8; j++) {
		if (img(i, j) != 0) {
			line1 = false;
			break;
		}
	}
	if (line1) {
		bool line2 = true;
		i = x + 2;
		for (j = y; j <= y + 5; j++) {
			if (img(i, j) != 0) {
				line2 = false;
				break;
			}
		}
		if (line2) {
			bool line3 = true;
			j = y + 6;
			for (i = x + 6; i <= x + 12; i++) {
				if (img(i, j) != 0) {
					line3 = false;
					break;
				}
			}
			if (line3) {
				bool line4 = true;
				j = y + 5;
				for (i = x + 9; i <= x + 16; i++) {
					if (img(i, j) != 0) {
						line4 = false;
						break;
					}
				}
				if (line4) {
					return true;
				}
			}
		}
	}
	return false;
}

PauseType getPauseType(Mat_<uchar> img, int x, int y) {
	if (!isStructuralElementInside(img, x, y, PAUSE)) {
		return P_INVALID;
	}

	if (isHalfPause(img, x, y)) {
		return P_HALF;
	}
	if (isQuarterPause(img, x, y)) {
		return P_QUARTER;
	}
	if (isEighthPause(img, x, y)) {
		return P_EIGHTH;
	}

	return P_INVALID;
}

NoteValue getNote(int staffIndex, int y, int lines[]) {

	int line1 = staffIndex * 5;
	int line2 = line1 + 1;
	int line3 = line1 + 2;
	int line4 = line1 + 3;
	int line5 = line1 + 4;

	if (y <= lines[line1] - 8) {
		return LA8;
	}
	else if (y <= lines[line1] - 3 && y >= lines[line1] - 7) {
		return SOL8;
	}
	else if (y <= lines[line1] + 2 && y >= lines[line1] - 2) {
		return FA8;
	}
	else if (y <= lines[line1] + 7 && y >= lines[line1] + 3) {
		return MI8;
	}
	else if (y <= lines[line2] + 2 && y >= lines[line2] - 2) {
		return RE8;
	}
	else if (y <= lines[line2] + 7 && y >= lines[line2] + 3) {
		return DO8;
	}
	else if (y <= lines[line3] + 2 && y >= lines[line3] - 2) {
		return SI;
	}
	else if (y <= lines[line3] + 7 && y >= lines[line3] + 3) {
		return LA;
	}
	else if (y <= lines[line4] + 2 && y >= lines[line4] - 2) {
		return SOL;
	}
	else if (y <= lines[line4] + 7 && y >= lines[line4] + 3) {
		return FA;
	}
	else if (y <= lines[line5] + 2 && y >= lines[line5] - 2) {
		return MI;
	}
	else if (y <= lines[line5] + 7 && y >= lines[line5] + 3) {
		return RE;
	}
	else if (y >= lines[line5] + 8) {
		return DO;
	}
	return UNDEFINED;
}

//

// VISUAL OUTPUT

void printNote(NoteValue noteValue, NoteType noteType) {
	char noteTypeString[15];
	char noteValueString[10];

	switch (noteType) {
	case WHOLE:
		strcpy(noteTypeString, "whole");
		break;
	case HALF:
		strcpy(noteTypeString, "half");
		break;
	case QUARTER:
		strcpy(noteTypeString, "quarter");
		break;
	case EIGHTH:
		strcpy(noteTypeString, "eighth");
		break;
	case SIXTEENTH:
		strcpy(noteTypeString, "sixteenth");
		break;
	default:
		strcpy(noteTypeString, "");
	}

	switch (noteValue) {
	case LA8:
		strcpy(noteValueString, "la 8");
		break;
	case SOL8:
		strcpy(noteValueString, "sol 8");
		break;
	case FA8:
		strcpy(noteValueString, "fa 8");
		break;
	case MI8:
		strcpy(noteValueString, "mi 8");
		break;
	case RE8:
		strcpy(noteValueString, "re 8");
		break;
	case DO8:
		strcpy(noteValueString, "do 8");
		break;
	case SI:
		strcpy(noteValueString, "si");
		break;
	case LA:
		strcpy(noteValueString, "la");
		break;
	case SOL:
		strcpy(noteValueString, "sol");
		break;
	case FA:
		strcpy(noteValueString, "fa");
		break;
	case MI:
		strcpy(noteValueString, "mi");
		break;
	case RE:
		strcpy(noteValueString, "re");
		break;
	case DO:
		strcpy(noteValueString, "do");
		break;
	default:
		strcpy(noteValueString, "");
	}

	cout << noteValueString << " " << noteTypeString << endl;
}

void printPause(PauseType pause) {
	char pauseString[15];
	switch (pause) {
	case P_HALF:
		strcpy(pauseString, "half pause");
		break;
	case P_QUARTER:
		strcpy(pauseString, "quarter pause");
		break;
	case P_EIGHTH:
		strcpy(pauseString, "eighth pause");
		break;
	default:
		strcpy(pauseString, "");
	}

	cout << pauseString << endl;
}

Mat_<Vec3b> contourNote(Mat_<Vec3b> img, int x, int y, Position position, Vec3b color) {
	Mat_<Vec3b> result(img.rows, img.cols);

	for (int i = 0; i < img.rows; i++) {
		for (int j = 0; j < img.cols; j++) {
			result(i, j) = img(i, j);
		}
	}

	if (position == N_A) { //whole note
		for (int i = x; i <= x + 10; i++) {
			int j = y;
			result(i, j) = color;
			j = y + 16;
			result(i, j) = color;
		}

		for (int j = y; j <= y + 16; j++) {
			int i = x;
			result(i, j) = color;
			i = x + 10;
			result(i, j) = color;
		}

		//point in middle:
		result(x + 4, y + 6) = color;
	}
	else if (position == DOWN) {

		for (int i = x + 30; i <= x + 41; i++) {
			int j = y;
			result(i, j) = color;
			j = y + 12;
			result(i, j) = color;
		}

		for (int j = y; j <= y + 12; j++) {
			int i = x + 30;
			result(i, j) = color;
			i = x + 41;
			result(i, j) = color;
		}

		//point in middle:
		result(x + 36, y + 6) = color;

	}
	else if (position == UP) {

		for (int i = x; i <= x + 9; i++) {
			int j = y;
			result(i, j) = color;
			j = y + 12;
			result(i, j) = color;
		}

		for (int j = y; j <= y + 12; j++) {
			int i = x;
			result(i, j) = color;
			i = x + 9;
			result(i, j) = color;
		}

		//point in middle:
		result(x + 4, y + 6) = color;

	}

	return result;
}

Mat_<Vec3b> contourPause(Mat_<Vec3b> img, int x, int y, PauseType pause, Vec3b color) {
	Mat_<Vec3b> result(img.rows, img.cols);

	for (int i = 0; i < img.rows; i++) {
		for (int j = 0; j < img.cols; j++) {
			result(i, j) = img(i, j);
		}
	}

	int pauseHeight = 0;
	int pauseWidth = 0;

	switch (pause) {
	case P_HALF:
		pauseHeight = HALF_PAUSE_HEIGHT;
		pauseWidth = HALF_PAUSE_WIDTH;
		break;
		break;
	case P_QUARTER:
		pauseHeight = QUARTER_PAUSE_HEIGHT;
		pauseWidth = QUARTER_PAUSE_WIDTH;
		break;
	case P_EIGHTH:
		pauseHeight = EIGHTH_PAUSE_HEIGHT;
		pauseWidth = EIGHTH_PAUSE_WIDTH;
		break;
	}

	for (int i = x; i <= x + pauseHeight; i++) {
		int j = y;
		result(i, j) = color;
		j = y + pauseWidth;
		result(i, j) = color;
	}

	for (int j = y; j <= y + pauseWidth; j++) {
		int i = x;
		result(i, j) = color;
		i = x + pauseHeight;
		result(i, j) = color;
	}

	return result;

}

//

// AUDIO OUTPUT

int speed = 1500;

void playNote(NoteValue noteValue, NoteType noteType) {
	int frequency = 0;
	int duration = 0;
	switch (noteType) {
	case WHOLE:
		duration = speed;
		break;
	case HALF:
		duration = speed / 2;
		break;
	case QUARTER:
		duration = speed / 4;
		break;
	case EIGHTH:
		duration = speed / 8;
		break;
	default:
		duration = 0;
	}

	switch (noteValue) {
	case LA8:

		break;
	case SOL8:
		frequency = 784;
		break;
	case FA8:
		frequency = 698;
		break;
	case MI8:
		frequency = 659;
		break;
	case RE8:
		frequency = 587;
		break;
	case DO8:
		frequency = 523;
		break;
	case SI:
		frequency = 494;
		break;
	case LA:
		frequency = 440;
		break;
	case SOL:
		frequency = 392;
		break;
	case FA:
		frequency = 349;
		break;
	case MI:
		frequency = 330;
		break;
	case RE:
		frequency = 294;
		break;
	case DO:
		frequency = 262;
		break;
	default:
		frequency = 0;
	}

	Beep(frequency, duration);
}

void playPause(PauseType pause) {
	int duration;

	switch (pause) {
	case P_HALF:
		duration = speed / 2;
		break;
	case P_QUARTER:
		duration = speed / 4;
		break;
	case P_EIGHTH:
		duration = speed / 8;
		break;
	default:
		duration = 0;
	}

	Sleep(duration);
}

//

// INPUT

void selectSpeed() {
	int speedNo = 0;
	do {
		cout << "Please select a speed:\n";
		cout << "1. Slow\n";
		cout << "2. Medium\n";
		cout << "3. Fast\n";
		cout << "Speed number: ";
		cin >> speedNo;
		switch (speedNo) {
		case 1:
			speed = 3000;
			break;
		case 2:
			speed = 2300;
			break;
		case 3:
			speed = 1500;
			break;
		default:
			cout << "Invalid speed. Please try again.\n";
		}
	} while (speedNo < 1 || speedNo > 3);
}

//

int main()
{
	bool exit = false;
	while (!exit) {
		cout << "Scores:\n";
		cout << "1. wholes.png - Only whole notes\n";
		cout << "2. halfs.png - Only half notes\n";
		cout << "3. quarters.png - Only quarter notes\n";
		cout << "4. eighth.png - Only eighth notes\n";
		cout << "5. jinglebells.png - Jingle Bells\n";
		cout << "\n";
		cout << "For exit, type 0\n\n";
		int score;
		cout << "Type in the number of the score you want: ";
		cin >> score;

		if (score == 0) {
			exit = true;
		}
		else if (score >= 1 && score <= 5) {
			selectSpeed();

			Mat_<Vec3b> img;

			switch (score) {
			case 1:
				img = imread("res/wholes.png");
				break;
			case 2:
				img = imread("res/halfs.png");
				break;
			case 3:
				img = imread("res/quarters.png");
				break;
			case 4:
				img = imread("res/eighths.png");
				break;
			case 5:
				img = imread("res/jinglebells.png");
				break;
			}

			Mat_<uchar> grayscale = convertToGrayscale(img);

			Mat_<uchar> binary = convertGrayscaleToBinary(grayscale);

			imshow("binary", binary);
			waitKey(0);

			//horizontal projection
			int* horizontalProjection = (int*)calloc(binary.rows, sizeof(int));
			for (int i = 0; i < binary.rows; i++) {
				for (int j = 0; j < binary.cols; j++) {
					if (binary(i, j) == 0) {
						horizontalProjection[i] += 1;
					}
				}
			}
			Mat_<Vec3b> imgHorizontalProjection(binary.rows, binary.cols);
			for (int i = 0; i < imgHorizontalProjection.rows; i++) {
				for (int j = 0; j < imgHorizontalProjection.cols; j++) {
					imgHorizontalProjection(i, j) = { 255, 255, 255 };
				}
			}
			for (int i = 0; i < binary.rows; i++) {
				line(imgHorizontalProjection, Point(0, i), Point(horizontalProjection[i], i), { 0, 0, 0 }, 1);
			}
			free(horizontalProjection);

			//imshow("horizontal projection", imgHorizontalProjection);
			//waitKey(0);
			//

			//get lines location
			Mat_<uchar> grayscaleProjection = convertToGrayscale(imgHorizontalProjection);

			int lines[20];
			int pos = 0;
			vector<pair<int, int>> staffs;
			int currentStaffStart;
			bool inStaff = false;
			int linesFound = 0;
			for (int i = 0; i < grayscaleProjection.rows; i++) {
				if (grayscaleProjection(i, 1) == 0 && !inStaff && linesFound == 0) {
					inStaff = true;
					currentStaffStart = i;
				}

				if (grayscaleProjection(i, grayscaleProjection.cols / 4) == 0) {
					lines[pos] = i;
					pos++;
					linesFound++;
				}

				if (grayscaleProjection(i, 1) != 0 && inStaff && linesFound == 5) {
					inStaff = false;
					staffs.push_back({ currentStaffStart, i });
					linesFound = 0;
				}
			}
			int nubmerOfStaffs = pos / 5;
			//

			//notes identification
			Mat_<Vec3b> result(binary.rows, binary.cols);

			for (int i = 0; i < img.rows; i++) {
				for (int j = 0; j < img.cols; j++) {
					uchar v = binary(i, j);
					result(i, j) = { v, v, v };
				}
			}

			for (int k = 0; k < staffs.size(); k++) {
				int staffStart = staffs[k].first;
				int staffEnd = staffs[k].second;
				for (int j = 0; j < img.cols; j++) {
					for (int i = staffStart; i < staffEnd; i++) {
						pair<NoteType, Position> note = getNoteType(binary, i, j);
						if (note.first != INVALID) {
							int notePosition = note.second == DOWN ? i + 36 : i + 4;
							NoteValue noteValue = getNote(k, notePosition, lines);
							printNote(noteValue, note.first);
							switch (note.first) {
							case WHOLE:
								result = contourNote(result, i, j, note.second, { 255, 255, 0 });
								break;
							case HALF:
								result = contourNote(result, i, j, note.second, { 0, 255, 0 });
								break;
							case QUARTER:
								result = contourNote(result, i, j, note.second, { 0, 0, 255 });
								break;
							case EIGHTH:
								result = contourNote(result, i, j, note.second, { 255, 0, 0 });
								break;
							}
							imshow("result", result);
							waitKey(1);
							playNote(noteValue, note.first);
							j += 3;
						}
						else {
							PauseType pause = getPauseType(binary, i, j);
							if (pause != P_INVALID) {
								printPause(pause);
								switch (pause) {
								case P_HALF:
									result = contourPause(result, i, j, pause, { 0, 255, 0 });
									break;
								case P_QUARTER:
									result = contourPause(result, i, j, pause, { 0, 0, 255 });
									break;
								case P_EIGHTH:
									result = contourPause(result, i, j, pause, { 255, 0, 0 });
									break;
								}
								imshow("result", result);
								waitKey(1);
								playPause(pause);
							}
						}
					}
				}
			}

			imshow("result", result);
			waitKey(0);

		}
		else {
			cout << "Invalid file! Please try again.";
		}
	}

	//
	return 0;
}