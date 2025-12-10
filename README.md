# Steiner-Ratio

`plot.cpp`：根据当前所有 split，计算 steiner ratio 是否大于等于 rho。split 的代码放在 `formulas` 文件夹中，`F0`,... 表示每次 improve 新增的 split。

`calc.py`：创建新的 `formulas/Fi`

- 如果是新增一个 regular point 存在的条件，把代码放在 `tmp/x_cond` 中，实现函数 `double X_cond(a2, a3, a4)` 和 `double AX_upper_bound(a2, a3, a4)`
- 如果是新增一个 s_plus，把 s_plus 放在 `tmp/s_plus` 中，把条件和 s_plus 的长度放在 `tmp/s_cond` 中，实现函数 `bool steiner_cond(b, c, d, s, e)` 和 `double steiner_length(b, c, d, s, e)`

`split_rho.cpp`：输入 `b, c, d, s, e`，计算所有 split 在该点处的 rho。注：这个文件只能在 Linux 下编译，Windows 上会编译失败。

`llm.py`：运行 LLM 寻找 regular point 存在的条件，或 s_plus 合法的条件。

