#pragma once

#include <app/model.h>
#include <app/timer.h>
#include <liim/vector.h>
#include <procinfo.h>

class ProcessModel final : public App::Model {
    APP_OBJECT(ProcessModel)

public:
    ProcessModel();

    enum Column {
        Name,
        Memory,
        Priority,
        RunningTime,
        __Count,
    };

    virtual int row_count() const override { return m_processes.size(); }
    virtual int col_count() const override { return Column::__Count; }
    virtual App::ModelData data(const App::ModelIndex& index) const override;
    virtual App::ModelData header_data(int col) const override;

    void load_data();

private:
    Vector<proc_info> m_processes;
    SharedPtr<App::Timer> m_timer;
};