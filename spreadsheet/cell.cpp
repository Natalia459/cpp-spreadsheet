#include "cell.h"

#include <cassert>
#include <iostream>
#include <string>

#include <memory>
#include <unordered_set>

#include <algorithm>

using namespace std::literals;

// Реализуйте следующие методы
//Cell::Cell()
//	:empty_cell_(std::nullopt), text_cell_(std::nullopt), formula_cell_(std::nullopt), impl_()
//{
//}

 Cell::Cell(Sheet* sheet)
	: impl_(), owner_sheet_(sheet) //empty_cell_(std::nullopt), text_cell_(std::nullopt), formula_cell_(std::nullopt),
{
}

Cell& Cell::operator=(Cell&& rhs) {
	if (this != &rhs) {
		//formula_cell_ = std::move(rhs.formula_cell_);
		//text_cell_ = std::move(rhs.text_cell_);
		impl_ = std::move(rhs.impl_);
		owner_sheet_ = std::move(rhs.owner_sheet_);
	}
	return *this;
}

Cell::Cell(Cell&& other)
	: impl_(std::move(other.impl_)), owner_sheet_(std::move(other.owner_sheet_)) //empty_cell_(), text_cell_(std::move(other.text_cell_)), formula_cell_(std::move(other.formula_cell_)),
{
}

void Cell::Set(std::string text) {
	if (text.empty()) {
		//if (!empty_cell_) {
		//	empty_cell_ = EmptyImpl{};
		//}
		EmptyImpl empty_cell = EmptyImpl{};
		impl_ = std::make_unique<EmptyImpl>(std::move(empty_cell));
		return;
	}
	if (text[0] == FORMULA_SIGN && text.size() != 1) {
		FormulaImpl	formula_cell = FormulaImpl{};
		try {
			//if (!formula_cell_) {
			//	formula_cell_ = FormulaImpl{};
			//}

			formula_cell.SetData(std::string{ text.begin() + 1, text.end() });
		}
		catch (const std::exception& exc) {
			std::throw_with_nested(FormulaException(exc.what()));
		}
		impl_ = std::make_unique<FormulaImpl>(std::move(formula_cell));
	}
	else {
		//if (!text_cell_) {
		//	text_cell_ = TextImpl{};
		//}
		TextImpl	text_cell = TextImpl{};
		text_cell.SetData(std::move(text));
		impl_ = std::make_unique<TextImpl>(std::move(text_cell));
	}
}

void Cell::SetDependences(Position ref_pos) {
	impl_->SetDependence(ref_pos);
}

void Cell::Clear() {
	//if (empty_cell_) {
	//	empty_cell_.reset();
	//}
	//if (text_cell_) {
	//	text_cell_.reset();
	//}
	//if (formula_cell_) {
	//	formula_cell_.reset();
	//}
	impl_.reset();
}


Cell::Value Cell::GetValue() const {
	if (!owner_sheet_) {
		throw;
	}
	return impl_->GetValue(reinterpret_cast<const SheetInterface&>(*owner_sheet_));
}
std::string Cell::GetText() const {
	return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
	return impl_->GetReferencedCells();
}

std::vector<Position> Cell::GetDependentCells() const {
	return impl_->GetDependentCells();
}

void Cell::CheckCyclicDependences() {
}

void Cell::InvalidateCache(Position pos) {
	impl_->InvalidateCache();
}

void Cell::DeleteDependence(Position pos) {
	impl_->DeleteDependence(pos);
}

bool Cell::IsReferenced() const {
	return !impl_->GetDependentCells().empty();
}

bool Cell::HasEmptyCache() const {
	return impl_->HasEmptyCache();
}

//std::vector<Position> Cell::FindDependedCells(Position pos) {
//	return owner_sheet_->GetCell(pos)->GetReferencedCells();
//}


void EmptyImpl::SetData(std::string&&) {
	InvalidateCache();
}

void EmptyImpl::SetDependence(Position ref_pos) {
	dependent_.push_back(ref_pos);
}

EmptyImpl::ImpValue EmptyImpl::GetValue(const SheetInterface&) const {
	if (cache_.has_value()) {
		return cache_.value();
	}
	cache_ = 0;
	return cache_.value();
}

std::string EmptyImpl::GetText() const {
	return empty_;
}

std::vector<Position> EmptyImpl::GetReferencedCells() const {
	return {};
}

std::vector<Position> EmptyImpl::GetDependentCells() const {
	return {};
}

void EmptyImpl::InvalidateCache() {
	if (cache_.has_value()) {
		cache_.reset();
	}
}
void EmptyImpl::DeleteDependence(Position pos) {
	dependent_.erase(std::find(dependent_.begin(), dependent_.end(), pos));
}

bool EmptyImpl::HasEmptyCache() const {
	return !cache_.has_value();
}



void TextImpl::SetData(std::string&& text) {
	text_ = std::move(text);
	InvalidateCache();
}

void TextImpl::SetDependence(Position ref_pos) {
	dependent_.push_back(ref_pos);
}

std::string TextImpl::GetText() const {
	return text_;
}

TextImpl::ImpValue TextImpl::GetValue(const SheetInterface&) const {
	if (cache_.has_value()) {
		return cache_.value();
	}
	if (text_[0] == ESCAPE_SIGN) {
		return std::string{ text_.begin() + 1, text_.end() };
	}
	return text_;
}

std::vector<Position> TextImpl::GetReferencedCells() const {
	return {};
}
std::vector<Position> TextImpl::GetDependentCells() const {
	return {};
}

void TextImpl::InvalidateCache() {
	if (cache_.has_value()) {
		cache_.reset();
	}
}

void TextImpl::DeleteDependence(Position pos) {
	dependent_.erase(std::find(dependent_.begin(), dependent_.end(), pos));
}

bool TextImpl::HasEmptyCache() const {
	return !cache_.has_value();
}



FormulaImpl& FormulaImpl::operator=(FormulaImpl&& rhs) {
	if (this != &rhs) {
		formula_ = std::move(rhs.formula_);
	}
	return *this;
}

FormulaImpl::FormulaImpl(FormulaImpl&& other)
	:formula_(std::move(other.formula_))
{
}

void FormulaImpl::SetData(std::string&& expression) {
	try {
		formula_ = ParseFormula(std::move(expression));
		InvalidateCache();
	}
	catch (const std::exception& exc) {
		std::throw_with_nested(FormulaException(exc.what()));
	}
}

void FormulaImpl::SetDependence(Position ref_pos) {

}

FormulaImpl::ImpValue FormulaImpl::GetValue(const SheetInterface& link) const {
	if (cache_.has_value()) {
		return cache_.value();
	}
	auto res = formula_->Evaluate(link);
	if (std::holds_alternative<double>(res)) {
		cache_ = std::get<double>(res);
		return cache_.value();
	}
	else {
		return std::get<FormulaError>(res);
	}
}

std::string FormulaImpl::GetText() const {
	return FORMULA_SIGN + formula_->GetExpression();
}

std::vector<Position> FormulaImpl::GetReferencedCells() const {
	return formula_.get()->GetReferencedCells();
}

std::vector<Position> FormulaImpl::GetDependentCells() const {
	return dependent_;
}

void FormulaImpl::DeleteDependence(Position pos) {
	dependent_.erase(std::find(dependent_.begin(), dependent_.end(), pos));
}

void FormulaImpl::InvalidateCache() {
	if (cache_.has_value()) {
		cache_.reset();
	}
}

bool FormulaImpl::HasEmptyCache() const {
	return !cache_.has_value();
}
