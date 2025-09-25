const axios = require('axios');

class SessionTester {
  constructor(baseUrl = 'http://localhost:8080') {
    this.baseUrl = baseUrl;
    this.sessionCookie = null;
  }

  async makeRequest(method, path, data = null) {
    const config = {
      method,
      url: `${this.baseUrl}${path}`,
      headers: {}
    };

    if (this.sessionCookie) {
      config.headers.Cookie = this.sessionCookie;
    }

    if (data) {
      config.data = data;
    }

    try {
      const response = await axios(config);
      
      // Extract session cookie
      if (response.headers['set-cookie']) {
        this.sessionCookie = response.headers['set-cookie'][0];
      }

      return response.data;
    } catch (error) {
      console.error(`Request failed: ${error.message}`);
      return null;
    }
  }

  async addToCart(item, price) {
    return await this.makeRequest('POST', '/cart/add', { item, price });
  }

  async getCart() {
    return await this.makeRequest('GET', '/cart');
  }

  async crashServer() {
    return await this.makeRequest('POST', '/crash');
  }
}

async function runDemo() {
  console.log('🛒 Starting Sticky Session Demo...\n');

  const tester = new SessionTester();

  // Add items to cart
  console.log('1. Adding items to cart...');
  await tester.addToCart('MacBook Pro', 2500);
  await tester.addToCart('iPhone 15', 1200);
  let cart = await tester.getCart();
  console.log('Cart after adding items:', cart);
  console.log(`Total: $${cart.total}\n`);

  // Simulate server crash
  console.log('2. Simulating server crash...');
  await tester.crashServer();

  // Wait a moment
  await new Promise(resolve => setTimeout(resolve, 2000));

  // Try to get cart after crash
  console.log('3. Trying to retrieve cart after crash...');
  cart = await tester.getCart();
  if (cart) {
    console.log('Cart after crash:', cart);
    console.log(`Total: $${cart.total}`);
    if (cart.cart.length === 0) {
      console.log('❌ CART LOST! This is the sticky session problem.');
    } else {
      console.log('✅ Cart survived! Stateless architecture works.');
    }
  } else {
    console.log('❌ Could not retrieve cart - server completely unavailable.');
  }
}

if (require.main === module) {
  runDemo().catch(console.error);
}
