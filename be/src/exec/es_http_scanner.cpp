// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#include "exec/es_http_scanner.h"

#include <sstream>
#include <iostream>

#include "runtime/descriptors.h"
#include "runtime/exec_env.h"
#include "runtime/mem_tracker.h"
#include "runtime/raw_value.h"
#include "runtime/tuple.h"
#include "exprs/expr.h"
#include "exec/text_converter.h"
#include "exec/text_converter.hpp"

namespace doris {

EsHttpScanner::EsHttpScanner(
            RuntimeState* state,
            RuntimeProfile* profile,
            TupleId tuple_id,
            std::map<std::string, std::string> properties,
            const std::vector<ExprContext*>& conjunct_ctxs,
            EsScanCounter* counter) :
        _state(state),
        _profile(profile),
        _tuple_id(tuple_id),
        _properties(properties),
        _conjunct_ctxs(conjunct_ctxs),
        _next_range(0),
        _line_eof(false),
#if BE_TEST
        _mem_tracker(new MemTracker()),
        _mem_pool(_mem_tracker.get()),
#else 
        _mem_tracker(new MemTracker(-1, "EsHttp Scanner", state->instance_mem_tracker())),
        _mem_pool(_state->instance_mem_tracker()),
#endif
        _tuple_desc(nullptr),
        _counter(counter),
        _es_reader(nullptr),
        _rows_read_counter(nullptr),
        _read_timer(nullptr),
        _materialize_timer(nullptr) {
}

EsHttpScanner::~EsHttpScanner() {
    close();
}

Status EsHttpScanner::open() {
    _tuple_desc = _state->desc_tbl().get_tuple_descriptor(_tuple_id);
    if (_tuple_desc == nullptr) {
        std::stringstream ss;
        ss << "Unknown tuple descriptor, tuple_id=" << _tuple_id;
        return Status(ss.str());
    }

    for (auto slot : _tuple_desc->slots()) {
        auto pair = _slots_map.emplace(slot->col_name(), slot);
        if (!pair.second) {
            std::stringstream ss;
            ss << "Failed to insert slot, col_name=" << slot->col_name();
            return Status(ss.str());
        }
    }

    const std::string& host = _properties.at(ESScanReader::KEY_HOST_PORT);
    _es_reader.reset(new ESScanReader(host, _properties));
    if (_es_reader == nullptr) {
        return Status("Es reader construct failed.");
    }

    _es_reader->open();

   //_text_converter.reset(new(std::nothrow) TextConverter('\\'));
   //if (_text_converter == nullptr) {
   //    return Status("No memory error.");
   //}

    _rows_read_counter = ADD_COUNTER(_profile, "RowsRead", TUnit::UNIT);
    _read_timer = ADD_TIMER(_profile, "TotalRawReadTime(*)");
    _materialize_timer = ADD_TIMER(_profile, "MaterializeTupleTime(*)");

    return Status::OK;
}

Status EsHttpScanner::get_next(Tuple* tuple, MemPool* tuple_pool, bool* eof) {
    SCOPED_TIMER(_read_timer);
    while (!eof) {
        std::string batch_row_buffer;
        if (_line_eof) {
            //RETURN_IF_ERROR(_es_reader->get_next(&eof, &batch_row_buffer));
        }
        //get_next_line(&batch_row_buffer);
        {
            COUNTER_UPDATE(_rows_read_counter, 1);
            SCOPED_TIMER(_materialize_timer);
           //if (convert_one_row(Slice(ptr, size), tuple, tuple_pool)) {
           //    break;
           //}
        }
    }
    return Status::OK;
}

void EsHttpScanner::close() {
    if (_es_reader != nullptr) {
        _es_reader->close();
    }
}

}