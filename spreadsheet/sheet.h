#pragma once

#include "cell.h"
#include "common.h"

#include <unordered_map>
#include <unordered_set>

class Sheet : public SheetInterface {
public:
    Sheet();
    ~Sheet();

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

private:
    std::unordered_map<int, std::unordered_map<int, std::unique_ptr<Cell>>> sheet_; //u_map<row, u_map<col, Cell*>> 
    Size min_size_;
    std::vector<int> rows_;
    std::vector<int> cols_;

    void SetDependence(Position ref_pos, Position parent);
    void ClearDependentCellCache(Position pos);
    void DeleteDependence(Position pos, std::vector<Position>&& prev_refs);

    void ExtractValue(std::ostream& output, const CellInterface::Value& val) const;
    bool CheckCellExistance(Position) const;
    void CheckCyclicDependences(Cell* copy_cell, Position pos);
    void SearchCyclicDependences(Cell* cell, std::unordered_set<Position, PositionHash>& unic_cells) const;

    void UpdateSize();
    void MakeHigherSize();
    void MakeLowerSize();
};