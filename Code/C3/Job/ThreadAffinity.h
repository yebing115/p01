#pragma once

namespace ThreadAffinity {
bool IsMainThread();
int GetWorkerThreadIndex();
void RegisterWorkerThread(int worker_index);
}
