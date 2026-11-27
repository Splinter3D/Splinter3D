# Task dispatching

How a background task travels from the UI to a worker thread, and how the
`Dispatcher` decides how many workers (`NPROC`) it gets. Sources:

- `src/application/tasks/task_manager.{hpp,cpp}`: queueing and ordering
- `src/platform/threading/dispatcher.{hpp,cpp}`: worker budgets, profiles
- `src/platform/threading/scalability_profile.{hpp,cpp}`: learning how an operation scales
- `src/platform/threading/worker_pool.{hpp,cpp}`: `parallelFor`
- `src/platform/benchmark/benchmark.hpp`: `measure()`

## 1. Task dispatch workflow

```mermaid
flowchart TD
    UI["UI / batch: TaskManager::submit(Task)"] --> Q["Entry state = Queued"]
    Q --> SCH["_schedule()"]

    SCH --> B1{"Earlier unfinished task<br/>writes a model this one<br/>reads or writes?"}
    B1 -- yes --> W1["Queued · waiting for task"]
    B1 -- no --> B2{"Has work()?"}
    B2 -- "no (instant edit)" --> APPLY["apply() inline, entry removed"]
    B2 -- yes --> B3{"launching > 0 or<br/>freeWorkers() == 0?"}
    B3 -- yes --> W2["Queued · waiting for workers"]
    B3 -- no --> START["_start: prepare() snapshots inputs<br/>on the UI thread, launching++"]

    W1 -. "_finish() reschedules" .-> SCH
    W2 -. "onCapacityFreed()" .-> SCH

    START --> SUB["Dispatcher::submit(operation, work, then)"]
    SUB --> POOL["WorkerPool thread"]
    POOL --> RUN["Dispatcher::run(operation, fn)"]

    subgraph DISP["Dispatcher::run"]
        direction TB
        ACQ["Lease → _acquire(operation)<br/>compute worker budget<br/>(see diagram 2)"]
        CTX["ExecutionContext{operation, budget}<br/>set on this thread"]
        MEAS["benchmark::measure(report, Time|CPUTime|CPUCores|RAM)<br/>sample before, run fn, sample after"]
        FN["work(context)"]
        REL["~Lease → _release(ticket, report, ok)"]
        ACQ --> CTX --> MEAS --> FN --> REL
    end

    RUN --> ACQ
    FN -. "NPROC / parallelFor()" .-> PF["WorkerPool::parallelFor<br/>slices = min(nproc, count)<br/>each slice gets share of budget"]

    REL --> LRN{"success and<br/>time ≥ 20 ms?"}
    LRN -- yes --> REC["profile.record(budget, report, contended)<br/>wall / cpu / cores / ram averages<br/>(EMA α=0.3, 0.1 if contended)"]
    LRN -- no --> SKIP["sample dropped"]
    REC --> SAVE[("splinter3D.profiles.json<br/>load() at start, save() later")]
    REL --> FREE["_allocated -= budget, _active--<br/>_capacityFreed() → listeners"]
    FREE -. "wakes" .-> SCH

    FREE --> THEN["then(): poster → _finish(id, error)"]
    PF -.-> MEAS
    THEN --> DONE["UI thread: apply() result, card removed<br/>_schedule() again"]
    START -. "work began: launching--, poster → _schedule()" .-> SCH
```

## 2. NPROC / worker budget computation (`_acquire`)

```mermaid
flowchart TD
    A["_acquire(operation)"] --> T["total = pool threads + 1<br/>pool = hardware_concurrency - 1<br/>(one core left for the UI)"]
    T --> P{"Called inside another run()?<br/>(parent ExecutionContext)"}
    P -- "yes (nested)" --> CAPN["cap = parent.budget"]
    P -- "no (top level)" --> CAPT["cap = max(1, total - allocated)"]

    CAPN --> PREF
    CAPT --> PREF
    PREF{"profile.preferredWorkers(total)<br/>available?"}

    PREF -- yes --> BP["budget = preferred"]
    PREF -- "no (never learned)" --> BN{"nested?"}
    BN -- yes --> BN1["budget = cap"]
    BN -- no --> BN2["budget = cap / (active + 1)<br/>fair share"]

    subgraph LEARN["preferredWorkers: ScalabilityProfile"]
        direction TB
        L1["per worker count p:<br/>speedup = clamp(avg cores, 1, p)"]
        L2["Karp–Flatt serial fraction<br/>s = (1/speedup − 1/p) / (1 − 1/p)<br/>weighted by runs, only p ≥ 2"]
        L3["Amdahl: pick the largest p ≤ total<br/>where adding one worker still gains ≥ 0.1 speedup"]
        L1 --> L2 --> L3
    end
    PREF -.-> LEARN

    BP --> EX
    BN1 --> CL
    BN2 --> CL
    EX{"runs % 8 == 7<br/>and preferred ≤ cap<br/>and cap > 1?"}
    EX -- yes --> EX1["explore: preferred ±1<br/>alternating up / down<br/>(forced up at 1, down at cap)"]
    EX -- no --> CL
    EX1 --> CL
    CL["budget = clamp(budget, 1, cap)"]
    CL --> TK{"nested?"}
    TK -- no --> TK1["allocated += budget, active++"]
    TK -- yes --> TK2["shares the parent's workers,<br/>nothing allocated"]
    TK1 --> OUT["Ticket{budget, contended = active > 1}"]
    TK2 --> OUT
    OUT --> N["NPROC = ExecutionContext.budget<br/>(1 outside any run)"]
```

## Notes

- `NPROC` is the `budget` of the current thread's `ExecutionContext`; it is 1
  outside any `run()`. `Dispatcher::run` sets the context from the ticket
  `_acquire` returns.
- An operation with no learned profile gets a fair share of the free workers,
  `cap / (active + 1)`. Once `ScalabilityProfile` has data, the worker count
  comes from Amdahl's law, and every 8th run tries one worker more or fewer
  (`kExploreEvery`).
- A top-level run that releases its workers fires `onCapacityFreed`, which wakes
  `TaskManager::_schedule` for tasks waiting for workers, whoever held them.
- Runs shorter than `kMinLearnedDuration` (20 ms) are not recorded: their CPU
  time mostly measures overhead.
