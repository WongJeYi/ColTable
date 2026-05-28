import time
import random
import string
import _ColTable as ct
import numpy as np
from typing import Type, TypeVar, List, Union
from ColTableAoA import View
NUM_INITIAL = 1000
NUM_TO_ADD = 500
LENGTH = 500

T = TypeVar("T", int, float, bool, string)

def random_generator(data_type: Type[T],loop) -> List[T]:
    #  random length
    length = LENGTH
    
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
def time_once(fn):
    start = time.perf_counter()
    result = fn()
    end = time.perf_counter()
    return result, end - start


def python_matmul(original):
    size = len(original)
    mat = [[0.0 for _ in range(size)] for _ in range(size)]

    for i in range(size):
        row = original[i]
        for j in range(size):
            total = 0.0
            for k in range(size):
                total += row[k] * original[k][j]
            mat[i][j] = total

    return mat

def coltable_scalar_matmul(double_view, size):
    mat = [[0.0 for _ in range(size)] for _ in range(size)]
    
    for i in range(size):
        for j in range(size):
            total = 0.0
            for k in range(size):
                total += double_view[i][k] * double_view[k][j]

            mat[i][j] = total

    return mat

def numpy_scalar_matmul(arr,n):
    mat = [[0.0 for _ in range(n)] for _ in range(n)]
    for i in range(n):
        for j in range(n):
            total = 0.0
            for k in range(n):
                total += arr[i, k] * arr[k, j]
            mat[i][j] = total

def multiplication2D_benchmark(data_type, loop):
    original = random_generator(data_type, loop)
    size = len(original)

    # Build ColTable before scalar computation timing
    if data_type is float:
        table = ct.ColTable.from_2d_double(original)
    elif data_type is int:
        table = ct.ColTable.from_2d_int64(original)
    elif data_type is bool:
        table = ct.ColTable.from_2d_bool(original)
    else:
        raise TypeError("Only numeric types are supported for matmul benchmark")

    # Normal Python computation only
    _, normal_compute = time_once(lambda: python_matmul(original))

    # ColTable scalar preparation
    Colarr, coltable_prepare = time_once(lambda: View(table, dtype=float))

    # ColTable scalar computation only
    _, coltable_scalar_compute = time_once(
        lambda: coltable_scalar_matmul(Colarr, size)
    )
    
    # NumPy preparation
    arr, numpy_prepare = time_once(lambda: np.array(original))
    
    #Numpy scalar computation only
    _, numpy_scalar_compute = time_once(
        lambda: numpy_scalar_matmul(arr,size)
    )

    # NumPy computation only
    _, numpy_compute = time_once(lambda: arr @ arr)

    # ColTable -> NumPy preparation
    matrix, coltable_numpy_prepare = time_once(lambda: table.to_numpy_matrix())

    # ColTable + NumPy computation only
    _, coltable_numpy_compute = time_once(lambda: matrix @ matrix)

    print("\nMultiplication Benchmark")
    print(f"Size: {size}")
    print("-" * 80)
    print(f"{'Method':<25} {'Computation':>15} {'Total':>15}")
    print("-" * 80)
    print(f"{'Normal Python':<25} {normal_compute:>15.6f} {normal_compute:>15.6f}")
    print(f"{'ColTable scalar':<25} {coltable_scalar_compute:>15.6f} {coltable_scalar_compute+coltable_prepare:>15.6f}")
    print(f"{'Numpy scalar':<25} {numpy_scalar_compute:>15.6f} {numpy_scalar_compute+numpy_prepare:>15.6f}")
    print(f"{'NumPy':<25} {numpy_compute:>15.6f} {(numpy_prepare + numpy_compute):>15.6f}")
    print(f"{'ColTable + NumPy':<25} {coltable_numpy_compute:>15.6f} {(coltable_numpy_prepare + coltable_numpy_compute):>15.6f}")

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
    struct_vec = generate_struct_data(NUM_INITIAL, max_list_size=LENGTH)
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
    list_col = table.get_child(key[0])

    results = []

    # --------------------------------------------------
    # long: ColTable + NumPy
    # --------------------------------------------------
    total_start = time.perf_counter()

    prep_start = time.perf_counter()
    arr_count = count_col.to_numpy()
    prep_end = time.perf_counter()

    compute_start = time.perf_counter()
    _ = arr_count.sum()
    compute_end = time.perf_counter()

    total_end = time.perf_counter()

    results.append({
        "data": "long",
        "method": "ColTable + NumPy",
        "preparation": prep_end - prep_start,
        "computation": compute_end - compute_start,
        "total": total_end - total_start,
    })

    # --------------------------------------------------
    # long: NumPy
    # --------------------------------------------------
    total_start = time.perf_counter()

    prep_start = time.perf_counter()
    np_count = np.array([row[key[1]] for row in structdata])
    prep_end = time.perf_counter()

    compute_start = time.perf_counter()
    _ = np_count.sum()
    compute_end = time.perf_counter()

    total_end = time.perf_counter()

    results.append({
        "data": "long",
        "method": "NumPy",
        "preparation": prep_end - prep_start,
        "computation": compute_end - compute_start,
        "total": total_end - total_start,
    })

    # --------------------------------------------------
    # list: ColTable + NumPy
    # --------------------------------------------------
    total_start = time.perf_counter()

    prep_start = time.perf_counter()
    arr_list = list_col.flattened_to_numpy()
    prep_end = time.perf_counter()

    compute_start = time.perf_counter()
    _ = arr_list.sum()
    compute_end = time.perf_counter()

    total_end = time.perf_counter()

    results.append({
        "data": "list",
        "method": "ColTable + NumPy",
        "preparation": prep_end - prep_start,
        "computation": compute_end - compute_start,
        "total": total_end - total_start,
    })

    # --------------------------------------------------
    # list: NumPy
    # --------------------------------------------------
    total_start = time.perf_counter()

    prep_start = time.perf_counter()
    np_list = np.array([x for row in structdata for x in row[key[0]]])
    prep_end = time.perf_counter()

    compute_start = time.perf_counter()
    _ = np_list.sum()
    compute_end = time.perf_counter()

    total_end = time.perf_counter()

    results.append({
        "data": "list",
        "method": "NumPy",
        "preparation": prep_end - prep_start,
        "computation": compute_end - compute_start,
        "total": total_end - total_start,
    })

    # --------------------------------------------------
    # Print table
    # --------------------------------------------------
    print("\nStruct / Column Benchmark")
    print("-" * 95)
    print(f"{'Data':<10} {'Method':<20} {'Preparation':>15} {'Computation':>15} {'Total':>15}")
    print("-" * 95)

    for r in results:
        print(
            f"{r['data']:<10} "
            f"{r['method']:<20} "
            f"{r['preparation']:>15.9f} "
            f"{r['computation']:>15.9f} "
            f"{r['total']:>15.9f}"
        )

    print("-" * 95)

    # Optional: print speedup summary
    long_coltable = results[0]["total"]
    long_numpy = results[1]["total"]
    list_coltable = results[2]["total"]
    list_numpy = results[3]["total"]

    print("\nSpeedup Summary")
    print("-" * 60)
    print(f"{'long total speedup':<30} {long_numpy / long_coltable:>10.2f}x")
    print(f"{'list total speedup':<30} {list_numpy / list_coltable:>10.2f}x")
    print("-" * 60)

    return results
if __name__ =="__main__":
    multiplication2D_benchmark(float,1)
    table,structdata,key=generate_struct_initial()
    countStruct(table,structdata,key)