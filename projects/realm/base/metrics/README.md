# Metrics contracts

## Storage and access

`MetricStorage<Id...>` owns the selected metrics in the listed order. Traits define their types,
initialization, and optional names; independent storage instances own independent values.

Compile-time `get<Id>()` and `getByName<"name">()` return references. Bind them with `auto&`;
metric objects cannot be copied or moved. Runtime `find<Metric>(id)` and
`findByName<Metric>(name)` return borrowed pointers, or `nullptr` for a missing entry or type mismatch.
Const storage exposes const references and pointers. Borrowed access must not outlive the storage.

## Concurrency

The policy belongs to each metric's type:

- `StoragePolicy::Unsafe` requires single-threaded access or external synchronization whenever
  a metric can be modified. This includes readers overlapping writers.
- `StoragePolicy::Atomic` uses relaxed, lock-free atomic operations on individual stored numbers.
  It supports concurrent observations and reads, but does not publish or synchronize unrelated data.
  It provides no snapshot spanning several numbers or several metrics.

A single counter, gauge, minimum, or maximum read loads one stored number. A mean reads its sum
and count separately, so during concurrent observations its result need not equal the mean of any
completed set of samples. Separate calls to `sum()` and `count()` are also not a consistent pair.
A histogram's `total()` loads its buckets separately; it need not agree with separately read bucket
counts. Collecting a consistent set of compound values requires externally synchronizing with writers,
even when using atomics.
Floating-point accumulation can still have rounding error and depend on observation order after
writers have stopped.

### Resetting a measurement interval

Atomic `reset()` operations do not cause data races, but are not a coordinated interval boundary.
An observation can overlap reset; mean sum/count and histogram buckets reset independently.
In particular, resetting a mean during an observation can leave a sum and count representing
different samples even after both operations finish.

For a coherent interval, callers must synchronize with all writers and readers before collecting
and resetting the relevant metrics. For example:

1. Wait for every job that accesses these metrics to finish, and prevent new accesses.
2. Collect values and reset the metrics.
3. Allow jobs for the next interval to access them.

A mutex around collection alone is insufficient unless every overlapping accessor participates in
that synchronization. Storage must also remain alive until all users have finished.

## Numeric behavior

Callers choose a sufficiently wide value type. Integer arithmetic results must remain representable
in their destination types, including internal sample counts and histogram totals. Overflow handling
is outside the supported contract: there are no overflow checks, saturation, or promised wrapping
semantics. Plain signed overflow can be undefined behavior; defined unsigned or atomic wrapping
does not extend the supported contract.

Floating-point samples are not checked for finiteness on the observation path:

| Metric | NaN behavior |
| --- | --- |
| Counter / gauge | Ordinary floating-point arithmetic propagates NaN; setting a gauge or resetting can replace it. |
| Mean | A NaN sample makes the sum and subsequent non-empty means NaN until reset. |
| Minimum / maximum | NaN observations do not replace a non-NaN stored value; initializing with NaN prevents ordinary comparisons from replacing it. |
| Histogram | NaN compares false against every bound and is counted in the final overflow bucket. |

Infinities follow ordinary arithmetic and comparisons. Histogram bounds must be finite and strictly
increasing, checked during compilation; negative infinity samples enter the first bucket and positive
infinity samples enter the overflow bucket. Bounds are inclusive upper limits and bucket counts are
per-bucket, not cumulative. Integer mean samples are accumulated in `f64`, so sufficiently large
integer values can lose precision on conversion.

Empty means return zero. Minimum and maximum metrics return their configured initial values until
observed; they do not separately track whether any samples have arrived. Reset restores the template
initial value, even if construction supplied a different starting value. Histogram reset clears counts
and preserves bounds.
