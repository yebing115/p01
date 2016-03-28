#pragma once

#define safe_delete(p) do { if (p) { delete p; p = nullptr; } } while (0)
