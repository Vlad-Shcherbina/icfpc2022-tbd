#![allow(dead_code)]

use std::sync::atomic::AtomicI64;
use std::sync::atomic::Ordering::Relaxed;
use std::sync::Mutex;
use std::collections::HashMap;
use std::fmt::Write as _;
use once_cell::sync::Lazy;

#[macro_export]
macro_rules! stats_counter {
    ($name:literal) => {
        {
            static TMP: once_cell::sync::Lazy<&$crate::stats::Counter> = once_cell::sync::Lazy::new(||
                $crate::stats::STATS.counter($name) 
            );
            *TMP
        }
    };
}

#[macro_export]
macro_rules! stats_timer {
    ($name:literal) => {
        {
            static TMP: once_cell::sync::Lazy<&$crate::stats::Timer> = once_cell::sync::Lazy::new(||
                $crate::stats::STATS.timer($name) 
            );
            *TMP
        }
    };
}

pub struct Stats {
    counters: Mutex<HashMap<&'static str, &'static Counter>>,
    timers: Mutex<HashMap<&'static str, &'static Timer>>,
}

impl Stats {
    pub fn counter(&self, name: &'static str) -> &'static Counter {
        STATS.counters.lock().unwrap()
            .entry(name)
            .or_insert_with(|| Box::leak(Box::default()))
    }

    pub fn timer(&self, name: &'static str) -> &'static Timer {
        STATS.timers.lock().unwrap()
            .entry(name)
            .or_insert_with(|| Box::leak(Box::default()))
    }

    #[allow(clippy::significant_drop_in_scrutinee)]
    pub fn render(&self) -> String {
        let mut result = String::new();
        let mut rows: Vec<(&str, i64, Option<i64>)> = vec![];
        for (name, counter) in STATS.counters.lock().unwrap().iter() {
            rows.push((name, counter.get(), None));
        }
        for (name, timer) in STATS.timers.lock().unwrap().iter() {
            rows.push((name, timer.count.get(), Some(timer.duration.get())));
        }
        let max: i64 = rows.iter().filter_map(|r| r.2).max().unwrap_or(0);

        rows.sort_by_key(|r| (r.0, r.2.is_some()));
        writeln!(result, "{:>12} {:>7}", "count", "reltime").unwrap();
        for (name, count, duration) in rows {
            let duration = match duration {
                None => String::new(),
                Some(duration) =>
                    format!("{:.2}%", 100.0 * duration as f64 / max as f64),
            };
            writeln!(result, "{count:>12} {duration:>7}  {name}").unwrap();
        }
        result
    }
}

pub static STATS: Lazy<Stats> = Lazy::new(|| Stats {
    counters: Mutex::new(HashMap::new()),
    timers: Mutex::new(HashMap::new()),
});

#[derive(Default)]
pub struct Counter(AtomicI64);

impl Counter {
    pub fn inc(&self) {
        self.inc_delta(1);
    }

    pub fn inc_delta(&self, delta: i64) {
        self.0.fetch_add(delta, Relaxed);
    }

    pub fn get(&self) -> i64 {
        self.0.load(Relaxed)
    }
}

#[derive(Default)]
pub struct Timer {
    count: Counter,
    duration: Counter,
}

impl Timer {
    pub fn time_it(&self) -> TimerGuard {
        TimerGuard::new(self)
    }
}

/*pub struct TimerGuard<'a> {
    timer: &'a Timer,
    start: std::time::Instant,
}

impl<'a> TimerGuard<'a> {
    fn new(timer: &'a Timer) -> TimerGuard<'a> {
        Self {
            timer,
            start: std::time::Instant::now(),
        }
    }
}

impl Drop for TimerGuard<'_> {
    fn drop(&mut self) {
        let duration = self.start.elapsed();
        self.timer.count.inc_delta(1);
        self.timer.duration.inc_delta(duration.as_nanos() as i64);
    }
}*/

pub struct TimerGuard<'a> {
    timer: &'a Timer,
    start: u64,
}

impl<'a> TimerGuard<'a> {
    fn new(timer: &'a Timer) -> TimerGuard<'a> {
        Self {
            timer,
            start: unsafe { core::arch::x86_64::_rdtsc() },
        }
    }
}

impl Drop for TimerGuard<'_> {
    fn drop(&mut self) {
        let d = unsafe { core::arch::x86_64::_rdtsc() } - self.start;
        self.timer.count.inc_delta(1);
        self.timer.duration.inc_delta(d as i64);
    }
}

pub fn benchmark() {
    const N: i32 = 10_000_000;

    fn ns(start: std::time::Instant) -> f64 {
        start.elapsed().as_secs_f64() * 1e9 / N as f64
    }

    let start = std::time::Instant::now();
    for _ in 0..N {
        let _t = STATS.timer("t1").time_it();
    }
    eprintln!("  hash lookup + timer took {:>6.2} ns", ns(start));

    let start = std::time::Instant::now();
    let t2 = STATS.timer("t2");
    for _ in 0..N {
        let _t = t2.time_it();
    }
    eprintln!("                timer took {:>6.2} ns", ns(start));

    let start = std::time::Instant::now();
    for _ in 0..N {
        STATS.counter("c1").inc_delta(42);
    }
    eprintln!("hash lookup + counter took {:>6.2} ns", ns(start));

    let start = std::time::Instant::now();
    let c2 = STATS.counter("t2");
    for _ in 0..N {
        c2.inc_delta(42);
    }
    eprintln!("              counter took {:>6.2} ns", ns(start));

    eprintln!("{}", STATS.render());
}
