import time
import random
import string
import _ColTable as ct
import numpy as np
from typing import Type, TypeVar, List, Union
NUM_INITIAL = 1000
NUM_TO_ADD = 500

T = TypeVar("T", int, float, bool, string)

def random_generator(data_type: Type[T],loop) -> List[T]:
    #  random length
    length = random.randint(1, 100)
    
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

def multiplication2D(data_type,loop):
    original= random_generator(data_type,loop)

    start_time = time.perf_counter()
    size = len(original)
    mat=[[0.0 for _ in range(size)] for _ in range(size)]
    for i in range(len(original)):
        for j in range(len(original[i])):
            for k in range(len(original[i])):
                mat[i][j]+=original[i][k]*original[k][j]

    end_time = time.perf_counter()
    print(f"Execution time Normal: {end_time - start_time:.6f} seconds")
    if(loop):
        if(data_type is int):
            table = ct.ColTable.from_2d_int64(original)
        elif(data_type is float):
            table = ct.ColTable.from_2d_double(original)
        elif(data_type is bool):
            table = ct.ColTable.from_2d_bool(original)
        elif(data_type is str):
            table = ct.ColTable.from_2d_string(original)
    else:
        if(data_type is int):
            table = ct.ColTable.from_int64(original)
        elif(data_type is float):
            table = ct.ColTable.from_double(original)
        elif(data_type is bool):
            table = ct.ColTable.from_bool(original)
        elif(data_type is str):
            table = ct.ColTable.from_string(original)

    start_time = time.perf_counter()
    size = len(original)
    mat=[[0.0 for _ in range(size)] for _ in range(size)]
    for i in range(len(original)):
        for j in range(len(original[i])):
            for k in range(len(original[i])):
                mat[i][j]+=table[i][k]*table[k][j]
    
    end_time = time.perf_counter()
    print(f"Execution time ColTable: {end_time - start_time:.6f} seconds")

    
    start_time = time.perf_counter()
    arr = np.array(original)
    result = arr @ arr
    
    end_time = time.perf_counter()
    print(f"Execution time Numpy Normal: {end_time - start_time:.6f} seconds")


    start_time = time.perf_counter()
    matrix = table.to_numpy_matrix()
    
    result = matrix @ matrix
    
    end_time = time.perf_counter()
    print(f"Execution time Numpy ColTable: {end_time - start_time:.6f} seconds")

def generate_random_list(size):
    return [random.randint(-1000, 1000) for _ in range(size)]

def generate_struct_data(num_rows, max_list_size=50):
    data = []
    for _ in range(num_rows):
        list_len = random.randint(0, max_list_size) # Random length for AoA
        data.append({
            "m_myList": generate_random_list(list_len),
            "m_count": list_len
        })
    return data

def python_insert_struct_column(table, indices, data_list, key, insert_method_name):
    child_table = table.get_child(key)
    
    extracted_column = [row[key] for row in data_list]
    print(f"\n--- DEBUG START ---")
    print(f"Child table '{key}' expects C++ type: {child_table.field.type_name}")
    print(f"--- DEBUG END ---\n")
    insert_func = getattr(child_table, insert_method_name)
    insert_func(indices, extracted_column)
    
    table.sync_length()

def generate_struct_initial():
    struct_vec = generate_struct_data(NUM_INITIAL, max_list_size=1000)
    empty_list_col = ct.ColTable.from_2d_long([])
    empty_count_col = ct.ColTable.from_long([])

    table = ct.ColTable.from_struct(
        [empty_list_col, empty_count_col], 
        ["m_myList", "m_count"]
    )
    initial_indices = list(range(len(struct_vec)))

    python_insert_struct_column(table, initial_indices, struct_vec, "m_myList", "insert_2d_at_long")
    python_insert_struct_column(table, initial_indices, struct_vec, "m_count", "insert_at_long")
    return table,struct_vec,["m_myList", "m_count"]
def countStruct(table, structdata, key):
    count_col = table.get_child(key[1])
    myList_col = table.get_child(key[0])

    # ColTable count conversion
    start = time.perf_counter()
    arr_count = count_col.to_numpy()
    end = time.perf_counter()
    print("ColTable count conversion:", end - start)

    # ColTable count query
    start = time.perf_counter()
    v = arr_count.sum()
    end = time.perf_counter()
    print("ColTable count sum:", end - start)

    # ColTable myList conversion
    start = time.perf_counter()
    arr_list = myList_col.flattened_to_numpy()
    end = time.perf_counter()
    print("ColTable myList flattened_to_numpy:", end - start)

    # ColTable myList query
    start = time.perf_counter()
    v = arr_list.sum()
    end = time.perf_counter()
    print("ColTable myList sum:", end - start)

    # Python/Numpy count conversion
    start = time.perf_counter()
    np_count = np.array([row[key[1]] for row in structdata])
    end = time.perf_counter()
    print("NumPy count conversion:", end - start)

    # Python/Numpy count query
    start = time.perf_counter()
    v = np_count.sum()
    end = time.perf_counter()
    print("NumPy count sum:", end - start)

    # Python/Numpy myList flatten + conversion
    start = time.perf_counter()
    np_list = np.array([x for row in structdata for x in row[key[0]]])
    end = time.perf_counter()
    print("NumPy myList flatten+conversion:", end - start)

    # Python/Numpy myList query
    start = time.perf_counter()
    v = np_list.sum()
    end = time.perf_counter()
    print("NumPy myList sum:", end - start)
    print(arr_count.dtype, arr_count.shape, arr_count.flags)
    print(np_count.dtype, np_count.shape, np_count.flags)

if __name__ =="__main__":
    multiplication2D(float,1)
    table,structdata,key=generate_struct_initial()
    countStruct(table,structdata,key)