#!/bin/bash

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

echo -e "${CYAN}═══════════════════════════════════════════════════════════${NC}"
echo -e "${CYAN}  IRQ System Analysis${NC}"
echo -e "${CYAN}═══════════════════════════════════════════════════════════${NC}"
echo

# Check IRQ distribution
echo -e "${YELLOW}Hardware Interrupt Distribution:${NC}"
echo "--------------------------------"
cat /proc/interrupts | head -20
echo

# Check softirq stats
echo -e "${YELLOW}Software Interrupt (Softirq) Stats:${NC}"
echo "------------------------------------"
cat /proc/softirqs
echo

# Check IRQ affinity for key interrupts
echo -e "${YELLOW}IRQ Affinity Settings:${NC}"
echo "----------------------"
for irq in /proc/irq/[0-9]*; do
    irq_num=$(basename "$irq")
    if [ -f "$irq/smp_affinity" ]; then
        affinity=$(cat "$irq/smp_affinity" 2>/dev/null)
        if [ ! -z "$affinity" ]; then
            echo "IRQ $irq_num: CPU mask $affinity"
        fi
    fi
done | head -10
echo

# Show interrupt context information
echo -e "${YELLOW}Interrupt Context Checks:${NC}"
echo "-------------------------"
echo "Current process can check interrupt context with in_interrupt()"
echo "Stack size in interrupt context: typically 16KB on x86-64"
echo "Interrupt stack location: /proc/interrupts shows per-CPU counts"
echo

# Show timing information
echo -e "${YELLOW}System Timing Information:${NC}"
echo "--------------------------"
echo "Timer interrupts (check LOC line in /proc/interrupts):"
grep "LOC:" /proc/interrupts | head -1
echo

# Check for interrupt storms
echo -e "${YELLOW}Checking for Interrupt Storms:${NC}"
echo "-------------------------------"
echo "Taking snapshots 1 second apart..."

snap1=$(cat /proc/interrupts)
sleep 1
snap2=$(cat /proc/interrupts)

echo "IRQs with >10,000 interrupts/sec may indicate storms:"
while IFS= read -r line; do
    if [[ $line =~ ^[[:space:]]*([0-9]+): ]]; then
        irq_num="${BASH_REMATCH[1]}"
        
        count1=$(echo "$snap1" | grep "^[[:space:]]*$irq_num:" | awk '{sum=0; for(i=2;i<=NF;i++){if($i~/^[0-9]+$/){sum+=$i}} print sum}')
        count2=$(echo "$snap2" | grep "^[[:space:]]*$irq_num:" | awk '{sum=0; for(i=2;i<=NF;i++){if($i~/^[0-9]+$/){sum+=$i}} print sum}')
        
        if [ ! -z "$count1" ] && [ ! -z "$count2" ]; then
            delta=$((count2 - count1))
            if [ $delta -gt 10000 ]; then
                name=$(echo "$line" | awk '{for(i=NF-2;i<=NF;i++)printf "%s ", $i}')
                echo -e "${RED}IRQ $irq_num: $delta/sec - $name${NC}"
            fi
        fi
    fi
done <<< "$snap2"

echo
echo -e "${GREEN}Analysis complete!${NC}"
