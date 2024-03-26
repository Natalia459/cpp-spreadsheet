#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;

namespace {
	class Formula : public FormulaInterface {
	public:
		explicit Formula(std::string expression) try
			:ast_(ParseFormulaAST(reinterpret_cast<const std::string&>(expression))) {
		}
		catch (const std::exception& exc) {
			std::throw_with_nested(FormulaException(exc.what()));
		}

		Value Evaluate(const SheetInterface& sheet) const override {
			auto func = [&](Position pos) {return (sheet.GetCell(pos)) ? sheet.GetCell(pos)->GetValue() : CellInterface::Value{0.0}; };
			try {
				return ast_.Execute(func);
			}
			catch (const FormulaError& exp) {
				return FormulaError(exp.GetCategory());
			}
		}

		std::string GetExpression() const override {
			std::ostringstream os;
			ast_.PrintFormula(os);
			return os.str();
		}

		std::vector<Position> GetReferencedCells() const override {
			auto cells = ast_.GetCells();
			std::vector<Position> sorted{ std::move_iterator(cells.begin()), std::move_iterator(cells.end()) };
			std::sort(sorted.begin(), sorted.end());
			sorted.erase(std::unique(sorted.begin(), sorted.end()), sorted.end());
			return sorted;
		}

	private:
		FormulaAST ast_;
	};
}// namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
	return std::make_unique<Formula>(std::move(expression));
}