use std::error::Error as StdError;
use std::sync::{Arc, Mutex};

use serde::{Deserialize, Serialize};
use tokio::fs::read_to_string;
use tokio::time::Duration;
use warp::Filter;

const LINK1_LOG: &str = "/home/djp/Rust/demo-server/python/fuck";
const LINK2_LOG: &str = "/home/djp/demo-server/python/fuck";
const LINK3_LOG: &str = "/home/djp/demo-server/python/fuck";
const LINK4_LOG: &str = "/home/djp/demo-server/python/fuck";

const SERVER_ADDR: &str = "172.25.45.190:1025";

#[derive(Debug, Serialize, Deserialize, Clone)]
struct NetStat {
    id: String,
    bits_received: u64,
    pkts_received: u64,
    pkts_missed: u64,
}

#[derive(Debug, Serialize, Deserialize, Clone)]
struct Edge {
    code: [i64; 4],
    description: [String; 4],
    port: [i64; 4],
    rt_bps: [u64; 4],
    rt_miss: [f64; 4],
    rt_miss_pkt: [u64; 4],
    rt_total_pkt: [u64; 4],
}

#[derive(Debug, Serialize, Deserialize)]
struct Response {
    edges: Vec<Edge>,
}

#[derive(Debug, Copy, Clone)]
pub struct IperfStat {
    bits_per_second: f64,
    pkts_received: u64,
    pkts_dropped: u64,
    drop_rate: f64,
}

#[derive(Debug, Deserialize)]
struct Arg {
    _arg: Option<String>,
}

impl IperfStat {
    pub fn from_str(line: &str) -> Option<IperfStat> {
        let mut char_stream = line.char_indices();

        if let Some((0, '[')) = char_stream.next() {
            let mut visited = String::new();
            let mut end_pos = 0;
            let mut_ref = &mut end_pos;

            loop {
                match char_stream.next() {
                    Some((pos, ']')) => {
                        // we find a curly brace pair.
                        *mut_ref = pos + 1;
                        break;
                    }
                    Some((_, c)) => {
                        visited.push(c);
                    }
                    None => return None,
                }
            }

            if line[end_pos..].contains("local")
                || line[end_pos..].contains("receiver")
                || line[end_pos..].contains("Interval")
            {
                return None;
            }

            let mut v = line[end_pos..].split_whitespace();

            let bits_per_second = v.nth(4)?.parse::<f64>().unwrap() * 1024.0 * 1024.0;

            let mut pkts = v.nth(3)?.split("/");
            let pkts_dropped = pkts.next().unwrap().parse::<u64>().unwrap();
            let pkts_received = pkts.next().unwrap().parse::<u64>().unwrap();

            return Some(IperfStat {
                bits_per_second,
                pkts_received,
                pkts_dropped,
                drop_rate: pkts_dropped as f64 / pkts_received as f64,
            });
        }

        None
    }
}

pub fn read_stats(file_string: &str, mut last_line_idx: usize) -> Option<(IperfStat, usize)> {
    let mut new_stat = IperfStat {
        bits_per_second: 0.0,
        pkts_received: 0,
        pkts_dropped: 0,
        drop_rate: 0.0,
    };

    for (line_idx, line) in file_string[..].trim().split("\n").enumerate() {
        if line_idx > last_line_idx {
            let res = IperfStat::from_str(line);
            match res {
                Some(stat) => {
                    new_stat = stat;
                    last_line_idx = line_idx;
                }
                None => {}
            }
        }
    }

    Some((new_stat, last_line_idx))
}

async fn keep_reading(shared_stat: Arc<Mutex<IperfStat>>, log_file_name: &str) {
    let mut last_line_idx = 0;
    loop {
        tokio::time::sleep(Duration::from_secs(5)).await;
        let file_string = read_to_string(log_file_name)
            .await
            .expect(&format!("log file {} is not available", log_file_name)[..]);
        let res = read_stats(&file_string[..], last_line_idx);
        match res {
            Some((stat, new_idx)) => {
                last_line_idx = new_idx;
                let mut guard = shared_stat.lock().unwrap();
                *guard = stat;
            }
            None => {}
        }
    }
}

async fn warp_handle(
    _arg: Arg,
    link_stats: Vec<Arc<Mutex<IperfStat>>>,
) -> Result<warp::reply::Json, warp::Rejection> {
    let link1_stat = link_stats[0].lock().unwrap();
    let link2_stat = link_stats[1].lock().unwrap();
    let link3_stat = link_stats[2].lock().unwrap();
    let link4_stat = link_stats[3].lock().unwrap();

    // build up the response
    let edge = Edge {
        code: [0, 0, 0, 0],
        description: [
            "success! Link is UP!".to_string(),
            "success! Link is UP!".to_string(),
            "success! Link is UP!".to_string(),
            "success! Link is UP!".to_string(),
        ],
        port: [1, 1, 1, 1],
        rt_bps: [
            link1_stat.bits_per_second as u64,
            link2_stat.bits_per_second as u64,
            link3_stat.bits_per_second as u64,
            link4_stat.bits_per_second as u64,
        ],
        rt_miss: [
            link1_stat.drop_rate,
            link2_stat.drop_rate,
            link3_stat.drop_rate,
            link4_stat.drop_rate,
        ],
        rt_miss_pkt: [
            link1_stat.pkts_dropped,
            link2_stat.pkts_dropped,
            link3_stat.pkts_dropped,
            link4_stat.pkts_dropped,
        ],
        rt_total_pkt: [
            link1_stat.pkts_received,
            link2_stat.pkts_received,
            link3_stat.pkts_received,
            link4_stat.pkts_received,
        ],
    };

    let response = Response { edges: vec![edge] };
    Ok(warp::reply::json(&response))
}

pub async fn run() -> Result<(), Box<dyn StdError>> {
    let default_stat = IperfStat {
        bits_per_second: 0.0,
        pkts_received: 0,
        pkts_dropped: 0,
        drop_rate: 0.0,
    };

    let link1_stat = Arc::new(Mutex::new(default_stat));
    let link2_stat = Arc::new(Mutex::new(default_stat));
    let link3_stat = Arc::new(Mutex::new(default_stat));
    let link4_stat = Arc::new(Mutex::new(default_stat));

    let _ = tokio::spawn(keep_reading(link1_stat.clone(), LINK1_LOG));
    let _ = tokio::spawn(keep_reading(link2_stat.clone(), LINK1_LOG));
    let _ = tokio::spawn(keep_reading(link3_stat.clone(), LINK1_LOG));
    let _ = tokio::spawn(keep_reading(link4_stat.clone(), LINK1_LOG));

    let warp_socket_addr = SERVER_ADDR
        .parse::<std::net::SocketAddr>()
        .expect("invalid warp listening address");

    let iperf_stat_provider = warp::any().map(move || {
        vec![
            link1_stat.clone(),
            link2_stat.clone(),
            link3_stat.clone(),
            link4_stat.clone(),
        ]
    });

    let route = warp::get()
        .and(warp::query())
        .and(iperf_stat_provider)
        .and_then(warp_handle);

    warp::serve(route).run(warp_socket_addr).await;

    Ok(())
}
