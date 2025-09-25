const axios = require('axios');

class SessionTester {
    constructor(baseUrl = 'http://localhost:8080') {
        this.baseUrl = baseUrl;
        this.sessionCookie = null;
    }
    
    async testStickySession() {
        console.log('\n🧪 Testing Sticky Session Implementation...');
        
        try {
            // Add items to cart
            const addResponse = await axios.post(`${this.baseUrl}/sticky/api/cart/add`, {
                productId: 'laptop-001',
                name: 'Gaming Laptop',
                price: 1299.99,
                quantity: 1
            });
            
            this.sessionCookie = addResponse.headers['set-cookie'];
            console.log('✅ Added item to cart:', addResponse.data);
            
            // Retrieve cart
            const getResponse = await axios.get(`${this.baseUrl}/sticky/api/cart`, {
                headers: { 'Cookie': this.sessionCookie?.[0] || '' }
            });
            
            console.log('✅ Retrieved cart:', getResponse.data);
            
            // Simulate server crash
            console.log('\n💥 Simulating server crash...');
            await axios.post(`${this.baseUrl}/sticky/crash`).catch(() => {
                console.log('Server crashed as expected');
            });
            
            // Try to retrieve cart after crash
            await new Promise(resolve => setTimeout(resolve, 2000));
            
            try {
                const postCrashResponse = await axios.get(`${this.baseUrl}/sticky/api/cart`, {
                    headers: { 'Cookie': this.sessionCookie?.[0] || '' }
                });
                console.log('❌ Unexpected: Cart survived crash!', postCrashResponse.data);
            } catch (error) {
                console.log('💔 Expected: Cart lost after server crash');
            }
            
        } catch (error) {
            console.error('Test error:', error.message);
        }
    }
    
    async testStatelessSession() {
        console.log('\n🧪 Testing Stateless Session Implementation...');
        
        try {
            // Add items to cart
            const addResponse = await axios.post(`${this.baseUrl}/stateless/api/cart/add`, {
                productId: 'laptop-002',
                name: 'Business Laptop',
                price: 899.99,
                quantity: 2
            });
            
            this.sessionCookie = addResponse.headers['set-cookie'];
            console.log('✅ Added items to cart:', addResponse.data);
            
            // Retrieve cart
            const getResponse = await axios.get(`${this.baseUrl}/stateless/api/cart`, {
                headers: { 'Cookie': this.sessionCookie?.[0] || '' }
            });
            
            console.log('✅ Retrieved cart:', getResponse.data);
            
            // Simulate server crash
            console.log('\n💥 Simulating server crash...');
            await axios.post(`${this.baseUrl}/stateless/crash`).catch(() => {
                console.log('Server crashed as expected');
            });
            
            // Try to retrieve cart after crash
            await new Promise(resolve => setTimeout(resolve, 3000));
            
            try {
                const postCrashResponse = await axios.get(`${this.baseUrl}/stateless/api/cart`, {
                    headers: { 'Cookie': this.sessionCookie?.[0] || '' }
                });
                console.log('🎉 SUCCESS: Cart survived crash!', postCrashResponse.data);
            } catch (error) {
                console.log('❌ Unexpected: Cart lost despite Redis storage');
            }
            
        } catch (error) {
            console.error('Test error:', error.message);
        }
    }
}

// Run tests
const tester = new SessionTester();

async function runAllTests() {
    console.log('🚀 Starting Session Management Tests');
    await tester.testStickySession();
    await tester.testStatelessSession();
    console.log('\n✅ All tests completed!');
}

if (require.main === module) {
    runAllTests();
}

module.exports = SessionTester;
