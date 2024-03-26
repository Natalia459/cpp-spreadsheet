#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::Sheet() {
	rows_.reserve(Position::MAX_ROWS);
	cols_.reserve(Position::MAX_COLS);
	rows_.push_back(0);
	cols_.push_back(0);
}

Sheet::~Sheet() = default;

//void Sheet::SetCell(Position pos, std::string text) {
//	if (!pos.IsValid()) {
//		throw InvalidPositionException{ "" };
//	}
//	Cell elem(this);
//	elem.Set(std::move(text));
//	std::vector<Position> prev_refs{};
//
//	std::string prev_val;
//	bool prev_value_exists = false;
//	if (CellExists(pos)) {
//		prev_val = sheet_[pos.col][pos.row]->GetText();
//		prev_value_exists = true;
//		prev_refs = sheet_[pos.row][pos.col]->GetReferencedCells();
//	}
//
//	std::unique_ptr<Cell> new_cell(std::make_unique<Cell>(std::move(elem)));
//	sheet_[pos.row].insert_or_assign(pos.col, std::move(new_cell));
//
//	try {
//		std::unordered_set<Position, PositionHash> unic_cells{};
//		CheckCyclicDependences(sheet_[pos.row][pos.col].get(), unic_cells);
//	}
//	catch (const CircularDependencyException& exp) {
//		if (prev_value_exists) {
//			Cell prev_elem(this);
//			prev_elem.Set(std::move(prev_val));
//			std::unique_ptr<Cell> prev_cell(std::make_unique<Cell>(std::move(prev_elem)));
//			sheet_[pos.row].insert_or_assign(pos.col, std::move(prev_cell));
//			for (auto ref_pos : sheet_[pos.row][pos.col]->GetReferencedCells()) {
//				SetDependence(pos, ref_pos);
//			}
//		}
//		std::throw_with_nested(CircularDependencyException{ exp.what() });
//	}
//	ClearDependentCellCache(pos);
//
//	if (!prev_refs.empty()) {
//		//если значение в €чейке уже существовало, то она могла ссылатьс€ на другие €чейки
//		// лишние св€зи нужно удалить
//		DeleteDependence(pos, prev_refs);
//	}
//
//	for (auto ref_pos : sheet_[pos.row][pos.col]->GetReferencedCells()) {
//		SetDependence(pos, ref_pos);
//	}
//
//	rows_.push_back(pos.row + 1);
//	cols_.push_back(pos.col + 1);
//	MakeHigherSize();
//}
void Sheet::SetCell(Position pos, std::string text) {
	if (!pos.IsValid()) {
		throw InvalidPositionException{ "" };
	}
	Cell elem(this);
	elem.Set(std::move(text));
	Cell copy_elem(this);

	bool prev_value_exists = false;
	if (CellExists(pos)) {
		prev_value_exists = true;
		copy_elem = std::move(*sheet_[pos.row][pos.col]);
	}

	std::unique_ptr<Cell> new_cell(std::make_unique<Cell>(std::move(elem)));
	sheet_[pos.row].insert_or_assign(pos.col, std::move(new_cell));

	try {
		std::unordered_set<Position, PositionHash> unic_cells{};
		CheckCyclicDependences(sheet_[pos.row][pos.col].get(), unic_cells);
	}
	catch (const CircularDependencyException& exp) {
		if (prev_value_exists) {
			std::unique_ptr<Cell> prev_cell(std::make_unique<Cell>(std::move(copy_elem)));
			sheet_[pos.row].insert_or_assign(pos.col, std::move(prev_cell));
			for (auto ref_pos : sheet_[pos.row][pos.col]->GetReferencedCells()) {
				SetDependence(pos, ref_pos);
			}
		}
		std::throw_with_nested(CircularDependencyException{ exp.what() });
	}
	ClearDependentCellCache(pos);

	if (prev_value_exists) {
		//если значение в €чейке уже существовало, то она могла ссылатьс€ на другие €чейки
		// лишние св€зи нужно удалить
		DeleteDependence(pos, copy_elem.GetReferencedCells());
	}

	for (auto ref_pos : sheet_[pos.row][pos.col]->GetReferencedCells()) {
		SetDependence(pos, ref_pos);
	}

	rows_.push_back(pos.row + 1);
	cols_.push_back(pos.col + 1);
	MakeHigherSize();
}

void Sheet::SetDependence(Position dependent, Position parent) {
	if (!CellExists(parent)) {
		Cell empty(this);
		empty.Set("");
		std::unique_ptr<Cell> new_cell(std::make_unique<Cell>(std::move(empty)));
		sheet_[parent.row].insert_or_assign(parent.col, std::move(new_cell));
		rows_.push_back(parent.row + 1);
		cols_.push_back(parent.col + 1);
		MakeHigherSize();
	}
	sheet_[parent.row][parent.col]->SetDependences(dependent);
}


const CellInterface* Sheet::GetCell(Position pos) const {
	if (!pos.IsValid()) {
		throw InvalidPositionException{ "" };
	}

	if (CellExists(pos)) {
		return sheet_.at(pos.row).at(pos.col).get();
	}
	return nullptr;
}

CellInterface* Sheet::GetCell(Position pos) {
	//return reinterpret_cast<CellInterface*>(GetCell(pos));
	if (!pos.IsValid()) {
		throw InvalidPositionException{ "" };
	}

	if (CellExists(pos)) {

		return sheet_[pos.row][pos.col].get();
	}
	return nullptr;
}

void Sheet::ClearCell(Position pos) {
	if (!pos.IsValid()) {
		throw InvalidPositionException{ "" };
	}

	if (CellExists(pos)) {
		ClearDependentCellCache(pos);
		//если значение в €чейке уже существовало, то она могла ссылатьс€ на другие €чейки
		// лишние св€зи нужно удалить
		std::vector<Position> copy_elem = sheet_[pos.row][pos.col]->GetReferencedCells();

		if (sheet_[pos.row].size() == 1) {
			sheet_.erase(pos.row);
		}
		else {
			sheet_[pos.row].erase(pos.col);
		}
		DeleteDependence(pos, std::move(copy_elem));

		auto pos_row = std::find(rows_.begin(), rows_.end(), pos.row + 1);
		rows_.erase(pos_row);
		auto pos_col = std::find(cols_.begin(), cols_.end(), pos.col + 1);
		cols_.erase(pos_col);
		MakeLowerSize();
	}
}

//удалить недействительный кэш
void Sheet::ClearDependentCellCache(Position pos) {
	auto deps = sheet_[pos.row][pos.col]->GetDependentCells();

	if (deps.empty()) {
		return;
	}
	for (auto [row, col] : deps) {
		if (CellExists({ row, col })) {
			if (sheet_[row][col]->HasEmptyCache()) {
				return;
			}
			sheet_[row][col]->InvalidateCache(pos);
			ClearDependentCellCache({ row, col });
		}
	}
}

void Sheet::DeleteDependence(Position pos, std::vector<Position>&& prev_refs) {
	std::vector<Position> new_refs;
	if (CellExists(pos)) {
		new_refs = sheet_[pos.row][pos.col]->GetReferencedCells();
	}
	std::vector<Position> diff;
	std::set_difference(prev_refs.begin(), prev_refs.end(), new_refs.begin(), new_refs.end(), std::back_inserter(diff));

	//удалить недействительные обратные зависимости
	for (auto d : diff) {
		if (CellExists(d)) {
			sheet_[d.row][d.col]->DeleteDependence(pos);
		}
	}
}

void Sheet::CheckCyclicDependences(Cell* cell, std::unordered_set<Position, PositionHash>& unic_cells) const {
	std::vector<Position> cells = cell->GetReferencedCells();
	if (cells.empty()) {
		return;
	}

	for (Position c : cells) {
		if (unic_cells.find(c) != unic_cells.end()) {
			throw CircularDependencyException{ "" };
		}
		unic_cells.insert(c);
		if (CellExists({ c.row, c.col })) {
			CheckCyclicDependences(sheet_.at(c.row).at(c.col).get(), unic_cells);
		}
	}
}

Size Sheet::GetPrintableSize() const {
	return min_size_;
}

void Sheet::PrintValues(std::ostream& output) const {
	//int sum = min_size_.rows + min_size_.cols;
	for (int row = 0; row < min_size_.rows; ++row) {
		for (int col = 0; col < min_size_.cols; ++col) {
			if (CellExists({ row, col })) {
				const auto& val = sheet_.at(row).at(col)->GetValue();
				ExtractValue(output, val);
			}
			if (col + 1 < min_size_.cols) {
				output << '\t';
			}
			//if (row + col + 2 != sum) {
			//	output << '\t';
			//}
		}
		output << '\n';
	}

	//for (auto& [row, _] : sheet_) {
	//	for (auto& [col, data] : sheet_.at(row)) {
	//		const auto& val = data->GetValue();
	//		ExtractValue(output, val);
	//	}
	//	output << '\n';
	//}
}

void Sheet::PrintTexts(std::ostream& output) const {
	//int sum = min_size_.rows + min_size_.cols;
	for (int row = 0; row < min_size_.rows; ++row) {
		for (int col = 0; col < min_size_.cols; ++col) {
			if (CellExists({ row, col })) {
				output << sheet_.at(row).at(col)->GetText();
			}
			if (col + 1 < min_size_.cols) {
				output << '\t';
			}
		}
		output << '\n';
	}

	//for (auto& [row, _] : sheet_) {
	//	for (auto& [col, data] : sheet_.at(row)) {
	//		output << data->GetText() << '\t';
	//	}
	//	output << '\n';
	//}
}

void Sheet::ExtractValue(std::ostream& output, const CellInterface::Value& val) const {
	if (std::holds_alternative<std::string>(val)) {
		output << std::get<std::string>(val);
	}
	if (std::holds_alternative<double>(val)) {
		output << std::get<double>(val);
	}
	if (std::holds_alternative<FormulaError>(val)) {
		output << std::get<FormulaError>(val);
	}
}

bool Sheet::CellExists(Position pos) const {
	if (sheet_.find(pos.row) != sheet_.end()) {
		if (sheet_.at(pos.row).find(pos.col) != sheet_.at(pos.row).end()) {
			return true;
		}
	}
	return false;
}

void Sheet::UpdateSize() {
	std::sort(rows_.begin(), rows_.end());
	std::sort(cols_.begin(), cols_.end());

	//Size prev_min = min_size_;
	//Size now = { pos.row + 1, pos.col + 1 };
	//bool changed = false;
	//if (min_size_.rows < rows_.back()) {
	//	min_size_.rows = rows_.back();
	//	//changed = true;
	//}
	//if (min_size_.cols < cols_.back()) {
	//	min_size_.cols = cols_.back();
	//	//changed = true;
	//}
	//
	//if (prev_size_.rows < now.rows && !changed) {
	//	prev_size_.rows = now.rows;
	//}
	//if (prev_size_.cols < now.cols && !changed) {
	//	prev_size_.cols = now.cols;
	//}
	//
	//if (changed) {
	//	if (prev_size_.rows < prev_min.rows) {
	//		prev_size_.rows = prev_min.rows;
	//	}
	//	if (prev_size_.cols < prev_min.cols) {
	//		prev_size_.cols = prev_min.cols;
	//	}
	//}
	//
	//if (now < prev_min && prev_min != min_size_) {
	//	prev_size_ = now;
	//}
	//if (prev_size_.rows < now.rows && min_size_.rows != now.rows) {
	//	prev_size_.rows = now.rows;
	//}
	//if (prev_size_.cols < now.cols && min_size_.cols != now.cols) {
	//	prev_size_.cols = now.cols;
	//}
}

void Sheet::MakeHigherSize() {
	UpdateSize();
	if (min_size_.rows < rows_.back()) {
		min_size_.rows = rows_.back();
		//changed = true;
	}
	if (min_size_.cols < cols_.back()) {
		min_size_.cols = cols_.back();
		//changed = true;
	}

}

void Sheet::MakeLowerSize() {
	UpdateSize();
	min_size_.rows = rows_.back();
	min_size_.cols = cols_.back();
}



std::unique_ptr<SheetInterface> CreateSheet() {
	return std::make_unique<Sheet>();
}