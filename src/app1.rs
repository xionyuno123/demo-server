use std::error::Error as StdError;
use std::sync::{Arc, Mutex};
use std::time::Instant;

use futures::stream::TryStreamExt;
use mongodb::bson::doc;
use mongodb::{options::ClientOptions, Client};
use serde::{Deserialize, Serialize};
use warp::Filter;

const SERVER_ADDR: &str = "172.25.45.190:1024";

#[derive(Debug, Serialize, Deserialize, Clone)]
struct NetStat {
    id: String,
    bits_received: u64,
    pkts_received: u64,
    pkts_missed: u64,
}

struct PrevValue {
    instant: Instant,
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

#[derive(Debug, Deserialize)]
struct Arg {
    _arg: Option<String>,
}

async fn read_from_db() -> Result<NetStat, Box<dyn StdError>> {
    let client_options = ClientOptions::parse("mongodb://localhost:27017").await?;
    let client = Client::with_options(client_options)?;
    let db = client.database("netstats-db");
    let collection = db.collection::<NetStat>("netstats");

    let filter = doc! { "id": "shared" };
    let mut cursor = collection.find(filter, None).await?;
    let mut doc_count = 0;
    let mut net_stat = NetStat {
        id: "shared".to_string(),
        bits_received: 0,
        pkts_received: 0,
        pkts_missed: 0,
    };

    while let Some(val) = cursor.try_next().await? {
        net_stat = val;
        doc_count += 1;
    }

    if doc_count > 1 {
        return Err("invalid database format".to_string().into());
    }

    if doc_count == 0 {
        // This is the first read, initialize it.
        let stats = vec![net_stat.clone()];
        collection.insert_many(stats, None).await?;
    }

    Ok(net_stat)
}

async fn warp_handle(
    _arg: Arg,
    prev_val: Arc<Mutex<PrevValue>>,
) -> Result<warp::reply::Json, warp::Rejection> {
    let curr_stat = read_from_db().await.map_err(|_| warp::reject())?;
    let mut guard = prev_val.lock().unwrap();

    let now = Instant::now();
    let interval = now.duration_since(guard.instant);
    println!("{:?}", interval.as_millis());

    // throughput and drop rate calculation
    let bits_speed = ((curr_stat.bits_received - guard.bits_received) as f64)
        / (interval.as_millis() as f64 / 1000.0);
    let drop_rate = if curr_stat.pkts_received - guard.pkts_received > 0 {
        ((curr_stat.pkts_missed - guard.pkts_missed) as f64)
            / ((curr_stat.pkts_received - guard.pkts_received) as f64)
    } else {
        0.0
    };

    // build up the response
    let edge = Edge {
        code: [0, -1, -1, -1],
        description: [
            "success! Link is UP!".to_string(),
            "Link is DOWN!".to_string(),
            "Link is DOWN!".to_string(),
            "Link is DOWN!".to_string(),
        ],
        port: [1, -1, -1, -1],
        rt_bps: [bits_speed as u64, 0, 0, 0],
        rt_miss: [drop_rate, 0.0, 0.0, 0.0],
        rt_miss_pkt: [curr_stat.pkts_missed - guard.pkts_missed, 0, 0, 0],
        rt_total_pkt: [curr_stat.pkts_received - guard.pkts_received, 0, 0, 0],
    };
    let response = Response { edges: vec![edge] };

    // Before quitting, update the guard
    guard.instant = now;
    guard.bits_received = curr_stat.bits_received;
    guard.pkts_received = curr_stat.pkts_received;
    guard.pkts_missed = curr_stat.pkts_missed;

    Ok(warp::reply::json(&response))
}

pub async fn run() -> Result<(), Box<dyn StdError>> {
    let stat = read_from_db().await?;
    println!(
        "MongoDB is correctly initialized with initial state: {:?}",
        &stat
    );

    let warp_socket_addr = SERVER_ADDR
        .parse::<std::net::SocketAddr>()
        .expect("invalid warp listening address");

    let prev_val = PrevValue {
        instant: Instant::now(),
        bits_received: stat.bits_received,
        pkts_received: stat.pkts_received,
        pkts_missed: stat.pkts_missed,
    };
    let prev_val = Arc::new(Mutex::new(prev_val));
    let prev_val_provider = warp::any().map(move || {
        let clone = prev_val.clone();
        clone
    });

    let route = warp::get()
        .and(warp::query())
        .and(prev_val_provider)
        .and_then(warp_handle);

    warp::serve(route).run(warp_socket_addr).await;

    Ok(())
}
