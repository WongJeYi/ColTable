#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include <pybind11/complex.h>
#include "ColTable.hpp"

namespace py = pybind11;

template <typename T>
void bind_coltable_factory(py::class_<ColTable, std::shared_ptr<ColTable>> &ct, const std::string &suffix) {
    
    // --- FROM PYTHON (Serialization) ---

    // 1. Flat Vector: ColTable.from_float([1.0, 2.0])
    ct.def_static(("from_" + suffix).c_str(), [](const std::vector<T>& v) {
        return ColTable::FromVector<T>(v);
    });

    // 2. 2d Vector (AoA): ColTable.from_arr_float([[1.0], [2.0]])
    ct.def_static(("from_2d_" + suffix).c_str(), [](const std::vector<std::vector<T>>& v) {
        return ColTable::FromVector<T>(v);
    });

    // 3. Flat std::list: ColTable.from_list_float([1.0, 2.0])
    ct.def_static(("from_list_" + suffix).c_str(), [](const std::list<T>& v) {
        return ColTable::FromVector<T>(v);
    });

    // 4. Vector of std::list: ColTable.from_arr_list_float([[1.0], [2.0]])
    // Fixed: Added <T> and corrected parentheses
    ct.def_static(("from_2d_list_" + suffix).c_str(), [](const std::vector<std::list<T>>& v) {
        return ColTable::FromVector<T>(v);
    });

    // --- TO PYTHON (Deserialization) ---

    // 5. To 1D Vector: obj.to_vector_float()
    ct.def(("to_vector_" + suffix).c_str(), [](std::shared_ptr<ColTable> self) {
        std::vector<T> out;
        ColTable::convertToArray<T>(self, out);
        return out;
    });

    // 6. To 1D List: obj.to_list_float()
    ct.def(("to_list_" + suffix).c_str(), [](std::shared_ptr<ColTable> self) {
        std::list<T> out;
        ColTable::convertToArray<T>(self, out);
        return out;
    });

    // 7. To 2D Vector: obj.to_2d_vector_float()
    ct.def(("to_2d_vector_" + suffix).c_str(), [](std::shared_ptr<ColTable> self) {
        std::vector<std::vector<T>> out;
        ColTable::convertToArray<T>(self, out);
        return out;
    });

    // 8. To Vector of Lists: obj.to_2d_list_float()
    ct.def(("to_2d_list_" + suffix).c_str(), [](std::shared_ptr<ColTable> self) {
        std::vector<std::list<T>> out;
        ColTable::convertToArray<T>(self, out);
        return out;
    });
    // 9. insert_at: insert for primitive vector
    ct.def(("insert_at_"+suffix).c_str(),[](ColTable& self, std::vector<int64_t>& indices, std::vector<T>& vec){
        self.insert_at(indices,vec);
    }, py::arg("indices"),py::arg("vec"),
    "indices is the array numbering of start_index\n vec represents the array to be inserted");
    // 10. insert AoA
    ct.def(("insert_2d_at_"+suffix).c_str(),[](ColTable& self, std::vector<int64_t>& indices,  std::vector<std::vector<T>>& vec){
        self.insert_at(indices,vec);
        std::cout << "Incoming Type: " << typeid(T).name() << std::endl;
    }, py::arg("indices"),py::arg("vec"),
    "indices is the array numbering of start_index\n vec represents the AoA to be inserted");
    // 11. get_child for use of insert column of struct
    ct.def("get_child", [](ColTable& self, const std::string& key) {
        for (size_t i = 0; i < self.field->children.size(); ++i) {
            if (self.field->children[i] && self.field->children[i]->key == key) {
                // Return a new ColTable wrapper pointing to the child buffers
                return std::make_shared<ColTable>(self.field->children[i], self.data->children[i],self.field->children[i]->expected_type);
            }
        }
        throw std::runtime_error("Column key not found in schema: " + key);
    });

    ct.def("sync_length_with_child", [](ColTable& self, const std::string& key) {
          for (size_t i = 0; i < self.getField()->children.size(); ++i) {
              if (self.getField()->children[i] != nullptr && self.getField()->children[i]->key == key) {
                  size_t child_len = self.getData()->children[i]->length;
                  if (child_len > self.getData()->length) {
                      self.getData()->length = child_len;
                  }
                  return;
              }
          }
    }, py::arg("key"), "Sync the length of the Struct table with its child");
     

}

#define BIND_CT_TYPE(type, name) bind_coltable_factory<type>(ct, name);

template <typename T>
void bind_view(py::module &m, const std::string &name) {
    py::class_<View<T>>(m, name.c_str(), py::buffer_protocol())
        .def_buffer([](View<T> &v) -> py::buffer_info {
            return py::buffer_info(
                v.data, sizeof(T),
                py::format_descriptor<T>::format(),
                1, { v.length }, { sizeof(T) }
            );
        });
}

void bind_ColTableData(py::module &m, const std::string &name){
    py::class_<ColTableData, std::shared_ptr<ColTableData>>(m, name.c_str())
        .def(py::init<size_t>())
        .def_readonly("length",&ColTableData::length)
        .def_readonly("nullCount",&ColTableData::nullCount)
        .def_readonly("validityBitmapsBuffer",&ColTableData::validityBitmapsBuffer)
        .def_readonly("nChildren",&ColTableData::nChildren)
        .def_readonly("offsetBuffer",&ColTableData::offsetBuffer)
        .def_readonly("valueBuffer",&ColTableData::valueBuffer)
        .def(py::init<const ColTableData &>())
        .def("addChild",&ColTableData::addChild)
        .def("addBuffer",&ColTableData::addBuffer)
        .def("addOffsetBuffer",&ColTableData::addOffsetBuffer)
        .def("removeChild",&ColTableData::removeChild)
        .def("getBuffer",&ColTableData::getBuffer,py::return_value_policy::reference_internal)
        .def("get_bool", &ColTableData::get<bool>, py::return_value_policy::reference_internal)
        .def("get_float", &ColTableData::get<float>, py::return_value_policy::reference_internal)
        .def("get_double", &ColTableData::get<double>, py::return_value_policy::reference_internal)
        .def("get_int64", &ColTableData::get<int64_t>, py::return_value_policy::reference_internal)
        .def("get_int32", &ColTableData::get<int32_t>, py::return_value_policy::reference_internal)
        .def("get_int16", &ColTableData::get<int16_t>, py::return_value_policy::reference_internal)
        .def("get_int8", &ColTableData::get<int8_t>, py::return_value_policy::reference_internal)
        .def("get_uint64", &ColTableData::get<uint64_t>, py::return_value_policy::reference_internal)
        .def("get_uint32", &ColTableData::get<uint32_t>, py::return_value_policy::reference_internal)
        .def("get_uint16", &ColTableData::get<uint16_t>, py::return_value_policy::reference_internal)
        .def("get_uint8", &ColTableData::get<uint8_t>, py::return_value_policy::reference_internal)
        //.def("get_short", &ColTableData::get<short>, py::return_value_policy::reference_internal)
        //.def("get_long", &ColTableData::get<long>, py::return_value_policy::reference_internal)
        //.def("get_long_long", &ColTableData::get<long long>, py::return_value_policy::reference_internal)
       .def("get_string_view", [](ColTableData &self) {
            return StringView{
                static_cast<char*>(self.valueBuffer->get()),
                static_cast<int64_t*>(self.offsetBuffer->get()),
                self.length,
                static_cast<uint8_t*>(self.validityBitmapsBuffer->get())
            };
        });
}
PYBIND11_MODULE(_ColTable, m) {
   // --- Floating Point ---
    bind_view<float>(m, "ViewFloat");
    bind_view<double>(m, "ViewDouble");

    // --- Bool ---
    bind_view<bool>(m,"ViewBool");

    // --- Explicitly Sized Signed Integers ---
    bind_view<int8_t>(m, "ViewInt8");
    bind_view<int16_t>(m, "ViewInt16");
    bind_view<int32_t>(m, "ViewInt32");
    bind_view<int64_t>(m, "ViewInt64");

    // --- Explicitly Sized Unsigned Integers ---
    bind_view<uint8_t>(m, "ViewUInt8");
    bind_view<uint16_t>(m, "ViewUInt16");
    bind_view<uint32_t>(m, "ViewUInt32");
    bind_view<uint64_t>(m, "ViewUInt64");

    // --- C++ Native Signed Integers ---
    //bind_view<short>(m, "ViewShort");
    //bind_view<long>(m, "ViewLong");
    //bind_view<long long>(m, "ViewLongLong");

    // --- C++ Native Unsigned Integers ---
    //bind_view<unsigned short>(m, "ViewUShort");
    //bind_view<unsigned long>(m, "ViewULong");
    //bind_view<unsigned long long>(m, "ViewULongLong");

    py::class_<StringView>(m, "ViewString")
        .def("__len__", [](const StringView &v) { return v.length; })
        .def("__getitem__", [](const StringView &v, size_t i) -> py::object {
            auto res = v.get_item(i);
            if (!res)  return py::none();
            return py::cast(*res);
        })
        .def("to_list", [](const StringView &v) {
            py::list l;
            for (size_t i = 0; i < v.length; ++i) {
                auto res = v.get_item(i);
                if (res) l.append(*res);
                else l.append(py::none());
            }
            return l;
        });
    py::enum_<format>(m, "Format")
        .value("BOOL", format::BOOL)
        .value("INT8", format::INT8)
        .value("INT16", format::INT16)
        .value("INT32", format::INT32)
        .value("INT64", format::INT64)
        .value("SHORT", format::SHORT)
        .value("LONG", format::LONG)
        .value("LONG_LONG", format::LONG_LONG)
        .value("UINT8", format::UINT8)
        .value("UINT16", format::UINT16)
        .value("UINT32", format::UINT32)
        .value("UINT64", format::UINT64)
        .value("UNSIGNED_SHORT", format::UNSIGNED_SHORT)
        .value("UNSIGNED_LONG", format::UNSIGNED_LONG)
        .value("UNSIGNED_LONG_LONG", format::UNSIGNED_LONG_LONG)
        .value("FLOAT", format::FLOAT)
        .value("DOUBLE", format::DOUBLE)
        .value("STRING", format::STRING)
        .value("LIST", format::LIST)
        .value("AoA", format::AoA)
        .value("STRUCT", format::STRUCT)
        .value("DICT", format::DICT)
        .value("UNKNOWN", format::UNKNOWN)
        .export_values();
    py::class_<ColTableField, std::shared_ptr<ColTableField>>(m, "ColTableField")
        .def(py::init([](std::string key, format f) {
            // Map the format enum to the correct C++ typeid for strict C++ type checking
            switch (f) {
                // Standard Integers
                case format::INT8:    return new ColTableField(key, f, typeid(int8_t));
                case format::INT16:   return new ColTableField(key, f, typeid(int16_t));
                case format::INT32:   return new ColTableField(key, f, typeid(int32_t));
                case format::INT64:   return new ColTableField(key, f, typeid(int64_t));
                
                // Unsigned Integers
                case format::UINT8:   return new ColTableField(key, f, typeid(uint8_t));
                case format::UINT16:  return new ColTableField(key, f, typeid(uint16_t));
                case format::UINT32:  return new ColTableField(key, f, typeid(uint32_t));
                case format::UINT64:  return new ColTableField(key, f, typeid(uint64_t));
                
                // Floating Point & Bool
                case format::FLOAT:   return new ColTableField(key, f, typeid(float));
                case format::DOUBLE:  return new ColTableField(key, f, typeid(double));
                case format::BOOL:    return new ColTableField(key, f, typeid(bool));
                
                // Strings (often represented as std::string or char*)
                case format::STRING:  return new ColTableField(key, f, typeid(std::string));
                
                // Nested Types (AoA = Array of Arrays / List)
                // For nested types, the parent usually tracks offsets, 
                // so void or a specialized nested type is used.
                case format::AoA:     
                case format::STRUCT:  
                    return new ColTableField(key, f, typeid(void)); 

                default:
                    // Fallback for custom or unknown formats
                    return new ColTableField(key, f, typeid(void));
            }
        }))
        .def_property_readonly("type_name", [](const ColTableField &self) {
            return std::string(self.expected_type.name());
        })
        .def_readonly("key",&ColTableField::key)
        .def_readonly("Format",&ColTableField::Format)
        .def_readonly("children",&ColTableField::children)
        .def_readonly("nChildren",&ColTableField::nChildren)
        .def(py::init<const ColTableField &>())
        .def("addChild",&ColTableField::addChild)
        .def("removeChild",&ColTableField::removeChild)
        .def("print",&ColTableField::print,py::arg("indent") = 0);
    
    bind_ColTableData(m,"ColTableData");
    auto ct = py::class_<ColTable, std::shared_ptr<ColTable>>(m, "ColTable");

    // Standard properties and constructors
    ct.def(py::init([](std::shared_ptr<ColTableField> f, std::shared_ptr<ColTableData> d) {
            return new ColTable(f, d, f->expected_type);
        }))
      .def_property_readonly("field", &ColTable::getField)
      .def_property_readonly("data", &ColTable::getData)
      .def("add", &ColTable::addChild, py::arg("other"))
      .def("remove", &ColTable::removeChild, py::arg("index"))
      .def("remove_at", [](ColTable& self, std::vector<int64_t> indices, size_t range) {
             self.remove_at(indices, range);
        }, py::arg("indices"),py::arg("range"),
        "Removes rows starting at the given indices.")
      .def("sync_length", &ColTable::sync_length_with_children);

    // --- The Smart Bindings ---
    BIND_CT_TYPE(bool,          "bool");
    BIND_CT_TYPE(float,          "float");
    BIND_CT_TYPE(double,         "double");
    BIND_CT_TYPE(int8_t,         "int8");
    BIND_CT_TYPE(int16_t,        "int16");
    BIND_CT_TYPE(int32_t,        "int32");
    BIND_CT_TYPE(int64_t,        "int64");
    BIND_CT_TYPE(uint8_t,        "uint8");
    BIND_CT_TYPE(uint16_t,       "uint16");
    BIND_CT_TYPE(uint32_t,       "uint32");
    BIND_CT_TYPE(uint64_t,       "uint64");
    BIND_CT_TYPE(std::optional<std::string> ,       "string");
    
    // Support for C++ native types (short, long)
    BIND_CT_TYPE(short,          "short");
    BIND_CT_TYPE(long,           "long");
    BIND_CT_TYPE(long long,      "longlong");

    // --- Special Cases (Non-Templated) ---
    ct.def_static("from_struct", &ColTable::FromStruct);
    ct.def_static("break_struct", [](std::shared_ptr<ColTable> table) {
        std::vector<ColTable> columns;
        ColTable::convertToStruct(table, columns);
        return columns;
    });


    
        
}