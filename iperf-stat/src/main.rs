
mod app1;
mod app2;

// Read and list main
#[tokio::main]
pub async fn main() -> Result<(), Box<dyn std::error::Error>> {
    app1::run().await
}
