import numpy as np
import _ColTable as ct
class View:
    TYPE_TO_SUFFIX = {
        float: "double",
        int: "long",
        bool: "bool",
    }

    def __init__(self, table, dtype=float, suffix=None, dense=True):
        if suffix is None:
            suffix = self.TYPE_TO_SUFFIX.get(dtype)
            if suffix is None:
                raise TypeError(f"Unsupported dtype: {dtype}")

        value_method = f"values_buffer_{suffix}"

        if not hasattr(table, value_method):
            raise AttributeError(
                f"ColTable does not have method {value_method}(). "
                f"Check your C++ binding suffix name."
            )

        self._values = memoryview(getattr(table, value_method)())
        self._offsets = memoryview(table.offsets_buffer())

        self.rows = len(self._offsets) - 1

        # Precompute row memoryviews once.
        self._rows = [
            self._values[self._offsets[i]:self._offsets[i + 1]]
            for i in range(self.rows)
        ]

    def __getitem__(self, i):
        # Supports double_view[i][j]
        return self._rows[i]

    def get(self, i, j):
        # Supports double_view.get(i, j)
        return self._rows[i][j]

    def row(self, i):
        return self._rows[i]

    def row_length(self, i):
        return len(self._rows[i])
