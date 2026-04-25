import pytest
import random
import string
import _ColTable as ct
from typing import Type, TypeVar, List, Union


T = TypeVar("T", int, float, bool, string)

def random_generator(data_type: Type[T],loop) -> List[T]:
    #  random length
    length = random.randint(1, 10)
    
    vec = []
    
    for _ in range(length):
        vec_inner=[]
        for _ in range(length):
            if data_type is float:
                vec_inner.append(random.uniform(0.0, 10.0))                
            elif data_type is bool:
                vec_inner.append(random.random() < 0.5)
            elif data_type is int:
                vec_inner.append(random.randint(-10, 10))
            elif data_type is str:
                chars = string.ascii_letters + string.digits
                vec_inner.append( ''.join(random.choices(chars, k=length)))    
            else:
                raise TypeError("Unsupported type for random_generator")
        if(loop):
            vec.append(vec_inner)
        else:
            vec=vec_inner
            break
                
    return vec

def round_trip_logic(data_type,loop):
    original= random_generator(data_type,loop)
    if(loop):
        if(data_type is int):
            table = ct.ColTable.from_2d_int64(original)
            recovered = table.to_2d_vector_int64()
        elif(data_type is float):
            table = ct.ColTable.from_2d_double(original)
            recovered = table.to_2d_vector_double()
        elif(data_type is bool):
            table = ct.ColTable.from_2d_bool(original)
            recovered = table.to_2d_vector_bool()
        elif(data_type is str):
            table = ct.ColTable.from_2d_string(original)
            recovered = table.to_2d_vector_string()
    else:
        if(data_type is int):
            table = ct.ColTable.from_int64(original)
            recovered = table.to_vector_int64()
        elif(data_type is float):
            table = ct.ColTable.from_double(original)
            recovered = table.to_vector_double()
        elif(data_type is bool):
            table = ct.ColTable.from_bool(original)
            recovered = table.to_vector_bool()
        elif(data_type is str):
            table = ct.ColTable.from_string(original)
            recovered = table.to_vector_string()

    assert len(recovered) == len(original), f"Size mismatch for type: {data_type}"
    assert recovered == original, f"Data mismatch for type: {data_type}"

# Using pytest parametrization to run the test for multiple types automatically
@pytest.mark.parametrize("data_type", [int, float, bool, str])
def test_all_types_round_trip(data_type):
    round_trip_logic(data_type,0)

@pytest.mark.parametrize("data_type", [int, float, bool, str])
def test_all_2d_types_round_trip(data_type):
    round_trip_logic(data_type,1)


def round_trip_logic_add_and_remove(data_type,loop):
    original= random_generator(data_type,loop)
    add=random_generator(data_type,loop)
    length=len(add)
    insert_index = len(original)
    if(loop):
        if(data_type is int):
            table = ct.ColTable.from_2d_int64(original)
            table.insert_2d_at_int64([insert_index],add)
            table.remove_at([insert_index],length)
            recovered = table.to_2d_vector_int64()
        elif(data_type is float):
            table = ct.ColTable.from_2d_double(original)
            table.insert_2d_at_double([insert_index],add)
            table.remove_at([insert_index],length)
            recovered = table.to_2d_vector_double()
        elif(data_type is bool):
            table = ct.ColTable.from_2d_bool(original)
            table.insert_2d_at_bool([insert_index],add)
            table.remove_at([insert_index],length)
            recovered = table.to_2d_vector_bool()
        elif(data_type is str):
            table = ct.ColTable.from_2d_string(original)
            table.insert_2d_at_string([insert_index],add)
            table.remove_at([insert_index],length)
            recovered = table.to_2d_vector_string()
    else:
        if(data_type is int):
            table = ct.ColTable.from_int64(original)
            table.insert_at_int64([insert_index],add)
            table.remove_at([insert_index],length)
            recovered = table.to_vector_int64()
        elif(data_type is float):
            table = ct.ColTable.from_double(original)
            table.insert_at_double([insert_index],add)
            table.remove_at([insert_index],length)
            recovered = table.to_vector_double()
        elif(data_type is bool):
            table = ct.ColTable.from_bool(original)
            table.insert_at_bool([insert_index],add)
            table.remove_at([insert_index],length)
            recovered = table.to_vector_bool()
        elif(data_type is str):
            table = ct.ColTable.from_string(original)
            table.insert_at_string([insert_index],add)
            table.remove_at([insert_index],length)
            recovered = table.to_vector_string()

    assert len(recovered) == len(original), f"Size mismatch for type: {data_type}"
    assert recovered == original, f"Data mismatch for type: {data_type}"

# Using pytest parametrization to run the test for multiple types automatically
@pytest.mark.parametrize("data_type", [int, float, bool, str])
def test_all_types_round_trip_add_and_remove(data_type):
    round_trip_logic_add_and_remove(data_type,0)

@pytest.mark.parametrize("data_type", [int, float, bool, str])
def test_all_2d_types_round_trip_add_and_remove(data_type):
    round_trip_logic_add_and_remove(data_type,1)

def generate_random_list(size):
    return [random.randint(-1000, 1000) for _ in range(size)]

def python_insert_struct_column(table, indices, data_list, key, insert_method_name):
    child_table = table.get_child(key)
    
    extracted_column = [row[key] for row in data_list]
    print(f"\n--- DEBUG START ---")
    print(f"Child table '{key}' expects C++ type: {child_table.field.type_name}")
    print(f"--- DEBUG END ---\n")
    insert_func = getattr(child_table, insert_method_name)
    insert_func(indices, extracted_column)
    
    table.sync_length()
def python_remove_struct_column(table, indices, data_list, key, remove_method_name):
    child_table = table.get_child(key)
    
    extracted_column = [row[key] for row in data_list]

    remove = getattr(child_table, remove_method_name)
    remove(indices, len(extracted_column))
    
    table.sync_length()


def test_struct_roundtrip_random_add_remove():
    
    struct_vec = [
        {"m_myList": generate_random_list(50), "m_count": 50},
        {"m_myList": generate_random_list(30), "m_count": 30}
    ]
    
    add_vec = [
        {"m_myList": generate_random_list(40), "m_count": 40},
        {"m_myList": generate_random_list(20), "m_count": 20}
    ]
    
    original= random_generator(int,1)
    empty_list_col= ct.ColTable.from_2d_long([])
    original= random_generator(int,0)
    empty_count_col= ct.ColTable.from_long([])
    table = ct.ColTable.from_struct(
        [empty_list_col, empty_count_col], 
        ["m_myList", "m_count"]
    )

    initial_indices = [0]
    python_insert_struct_column(table, initial_indices, struct_vec, "m_myList", "insert_2d_at_long")
    python_insert_struct_column(table, initial_indices, struct_vec, "m_count", "insert_at_long")

    insert_indices = [len(struct_vec)]
    python_insert_struct_column(table, insert_indices, add_vec, "m_myList", "insert_2d_at_long")
    python_insert_struct_column(table, insert_indices, add_vec, "m_count", "insert_at_long")

    python_remove_struct_column(table, insert_indices, add_vec, "m_myList", "remove_at")
    python_remove_struct_column(table, insert_indices, add_vec, "m_count", "remove_at")

    assert table.data.length == len(struct_vec), "Size mismatch after insert and remove!"