#include "common.h"

#include <cctype>
#include <sstream>

#include <array>
#include <vector>

const int ALPHA_SIZE = 26;
const int MAX_POSITION_LENGTH = 8;
const int ANSI_A_INDEX = 65;

const Position Position::NONE = { -1, -1 };


//FormulaError::Category FormulaError::GetCategory() const {
//	return category_;
//}
//
//bool FormulaError::operator==(FormulaError rhs) const {
//	return category_ == rhs.category_;
//}
//
//std::string_view FormulaError::ToString() const {
//	return "#ARITHM!";
//}



bool Position::operator==(const Position rhs) const {
	return rhs.row == row && rhs.col == col;
}

bool Position::operator<(const Position rhs) const {
	return col <= rhs.col ? row < rhs.row : false;
}

bool Position::IsValid() const {
	static const int MAX_VAL = 16384;
	bool negative = col < 0 || row < 0;
	bool limited = row < MAX_VAL && col < MAX_VAL;
	return limited ? !negative : false;
}

std::string Position::ToString() const {
	if (!IsValid()) {
		return "";
	}
	std::vector<char> letters;
	int c = col;
	do {
		letters.push_back(c % ALPHA_SIZE + ANSI_A_INDEX);
		c /= ALPHA_SIZE;
		--c;
	} while (c >= 0);
	std::string result(letters.rbegin(), letters.rend());
	result += std::to_string(row + 1);
	return result;
}

Position Position::FromString(std::string_view str) {
	if (str.empty() || str.size() > MAX_POSITION_LENGTH) {
		return NONE;
	}
	size_t i = 0;
	while (std::isalpha(str[i])) {
		++i;
		if (i == str.size()) {
			return NONE;
		}
	}
	Position res;
	std::string_view letters = str.substr(0, i);
	res.col = letters.empty() ? -1 : 0;
	int pow = 1;
	bool is_not_first = false;
	for (auto it = letters.rbegin(); it < letters.rend(); ++it) {
		if (*it - ANSI_A_INDEX > ALPHA_SIZE) {
			res = NONE;
			break;
		}
		res.col += (*it - ANSI_A_INDEX + is_not_first) * pow;
		pow *= ALPHA_SIZE;
		is_not_first = true;
	}
	std::string_view numbers = str.substr(i, str.npos);
	for (auto it = numbers.begin(); it < numbers.end(); ++it) {
		if (!std::isdigit(*it)) {
			return NONE;
		}
	}
	res.row = std::stoi(std::string(numbers));
	if (res.row <= 0) {
		return NONE;
	}
	return { res.row - 1, res.col };
}

bool Size::operator==(Size rhs) const {
	return rows == rhs.rows && cols == rhs.cols;
}