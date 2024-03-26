#pragma once

#include "common.h"
#include "formula.h"

#include <functional>
#include <optional>
#include <unordered_set>

class Impl {
public:
	using ImpValue = std::variant<std::string, double, FormulaError>;

	virtual ~Impl() = default;
	virtual void SetData(std::string&&) = 0;
	virtual void SetDependence(Position ref_pos) = 0;
	virtual ImpValue GetValue(const SheetInterface& link) const = 0;
	virtual std::string GetText() const = 0;
	virtual std::vector<Position> GetReferencedCells() const = 0;
	virtual std::vector<Position> GetDependentCells() const = 0;
	virtual void DeleteDependence(Position pos) = 0;
	virtual void InvalidateCache() = 0;
	virtual bool HasEmptyCache() const = 0;
};

class Cell : public CellInterface {
public:
	Cell(Sheet* sheet);
	//Cell(Sheet& sheet);

	Cell(const Cell&) = delete;
	Cell& operator=(const Cell&) = delete;

	Cell(Cell&&);
	Cell& operator=(Cell&&);
	~Cell() = default;

	void Set(std::string text);
	void SetDependences(Position ref_pos);
	void Clear();

	Value GetValue() const override;
	std::string GetText() const override;

	std::vector<Position> GetReferencedCells() const override;
	std::vector<Position> GetDependentCells() const;
	bool IsReferenced() const;
	void DeleteDependence(Position pos);
	void InvalidateCache(Position pos);
	bool HasEmptyCache() const;

private:
	std::unique_ptr<Impl> impl_;
	Sheet* owner_sheet_;

	void CheckCyclicDependences();
	//std::vector<Position> FindDependedCells(Position pos);
};

class EmptyImpl : public Impl {
public:
	EmptyImpl() = default;

	void SetData(std::string&&) override;
	void SetDependence(Position ref_pos);

	ImpValue GetValue(const SheetInterface& link) const override;
	std::string GetText() const override;
	std::vector<Position> GetReferencedCells() const override;
	std::vector<Position> GetDependentCells() const override;
	void DeleteDependence(Position pos);
	void InvalidateCache();
	bool HasEmptyCache() const;

private:
	std::string empty_ = "";
	mutable std::optional<double> cache_;
	std::vector<Position> dependent_; // if few cells have reference to this cell
};

class TextImpl : public Impl {
public:
	TextImpl() = default;
	void SetData(std::string&& text) override;
	void SetDependence(Position ref_pos);

	std::string GetText() const override;
	ImpValue GetValue(const SheetInterface& link) const override;
	std::vector<Position> GetReferencedCells() const override;
	std::vector<Position> GetDependentCells() const override;
	void DeleteDependence(Position pos);
	void InvalidateCache();
	bool HasEmptyCache() const;

private:
	std::string text_;
	mutable std::optional<double> cache_;
	std::vector<Position> dependent_; // if few cells have reference to this cell
};

class FormulaImpl : public Impl {
public:
	FormulaImpl() = default;

	FormulaImpl(const FormulaImpl&) = delete;
	FormulaImpl& operator=(const FormulaImpl& rhs) = delete;

	FormulaImpl(FormulaImpl&&);
	FormulaImpl& operator=(FormulaImpl&& rhs);

	void SetData(std::string&& expression) override;
	void SetDependence(Position ref_pos);
	ImpValue GetValue(const SheetInterface& link) const override;
	std::string GetText() const override;
	std::vector<Position> GetReferencedCells() const override;
	std::vector<Position> GetDependentCells() const override;
	void DeleteDependence(Position pos);
	void InvalidateCache();
	bool HasEmptyCache() const;

private:
	std::unique_ptr<FormulaInterface> formula_;
	mutable std::optional<double> cache_;
	std::vector<Position> dependent_; // if few cells have reference to this cell
};

